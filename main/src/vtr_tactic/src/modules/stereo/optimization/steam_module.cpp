#include <vtr_tactic/modules/stereo/optimization/steam_module.hpp>

namespace vtr {
namespace tactic {

bool checkDiagonal(Eigen::Array<double, 1, 6> &diag) {
  for (int idx = 0; idx < 6; ++idx) {
    if (diag(idx) <= 0) return false;
  }
  return true;
}

void SteamModule::setConfig(std::shared_ptr<Config> &config) {
  config_ = config;

  backup_params_.verbose = config_->verbose;
  backup_params_.maxIterations = config_->iterations;
  backup_params_.absoluteCostThreshold = config_->absoluteCostThreshold;
  backup_params_.absoluteCostChangeThreshold =
      config_->absoluteCostChangeThreshold;
  backup_params_.relativeCostChangeThreshold =
      config_->relativeCostChangeThreshold;

  backup_params_.shrinkCoeff = config_->shrinkCoeff;
  backup_params_.growCoeff = config_->growCoeff;
  backup_params_.maxShrinkSteps = config_->maxShrinkSteps;

  Eigen::Array<double, 1, 6> Qc_diag;
  Qc_diag << config_->lin_acc_std_dev_x, config_->lin_acc_std_dev_y,
      config_->lin_acc_std_dev_z, config_->ang_acc_std_dev_x,
      config_->ang_acc_std_dev_y, config_->ang_acc_std_dev_z;
  if (checkDiagonal(Qc_diag) == false && config_->trajectory_smoothing) {
    throw std::runtime_error(
        "Elements of the smoothing factor must be greater than zero!");
  }
  // Make Qc_inv
  smoothing_factor_information_.setZero();
  smoothing_factor_information_.diagonal() = 1.0 / Qc_diag;

  // Setup velocity prior
  velocity_prior_ << config_->lin_vel_mean_x, config_->lin_vel_mean_y,
      config_->lin_vel_mean_z, config_->ang_vel_mean_x, config_->ang_vel_mean_y,
      config_->ang_vel_mean_z;

  Eigen::Array<double, 1, 6> Qv_diag;
  Qv_diag << config_->lin_vel_std_dev_x, config_->lin_vel_std_dev_y,
      config_->lin_vel_std_dev_z, config_->ang_vel_std_dev_x,
      config_->ang_vel_std_dev_y, config_->ang_vel_std_dev_z;

  if (checkDiagonal(Qv_diag) == false && config_->trajectory_smoothing) {
    throw std::runtime_error(
        "Error: elements of the velocity prior noise must be greater than "
        "zero!");
  }
  velocity_prior_cov_.setZero();
  velocity_prior_cov_.diagonal() = 1.0 / Qv_diag;

  // temporary way to set up GPS-vehicle transform
  Eigen::Vector3d r_vg_g, phi;
  r_vg_g << config_->tf_gv_x, config_->tf_gv_y, config_->tf_gv_z;
  phi << config_->tf_gv_phi1, config_->tf_gv_phi2, config_->tf_gv_phi3;
  tf_gps_vehicle_ = steam::se3::FixedTransformEvaluator::MakeShared(
      lgmath::se3::Transformation(
          lgmath::so3::Rotation(phi).matrix(), r_vg_g).inverse());
}

std::shared_ptr<steam::SolverBase> SteamModule::generateSolver(
    std::shared_ptr<steam::OptimizationProblem> &problem) {
  // Setup Solver
  std::shared_ptr<steam::SolverBase> solver;
  if (config_->solver_type == "LevenburgMarquardt") {
    steam::LevMarqGaussNewtonSolver::Params params;
    params.verbose = config_->verbose;
    params.maxIterations = config_->iterations;
    params.absoluteCostThreshold = config_->absoluteCostThreshold;
    params.absoluteCostChangeThreshold = config_->absoluteCostChangeThreshold;
    params.relativeCostChangeThreshold = config_->relativeCostChangeThreshold;

    params.shrinkCoeff = config_->shrinkCoeff;
    params.growCoeff = config_->growCoeff;
    params.maxShrinkSteps = config_->maxShrinkSteps;

    solver.reset(new steam::LevMarqGaussNewtonSolver(problem.get(), params));
  } else if (config_->solver_type == "DoglegGaussNewton") {
    steam::DoglegGaussNewtonSolver::Params params;
    params.verbose = config_->verbose;
    params.maxIterations = config_->iterations;
    params.absoluteCostThreshold = config_->absoluteCostThreshold;
    params.absoluteCostChangeThreshold = config_->absoluteCostChangeThreshold;
    params.relativeCostChangeThreshold = config_->relativeCostChangeThreshold;

    params.ratioThresholdShrink = config_->ratioThresholdShrink;
    params.ratioThresholdGrow = config_->ratioThresholdGrow;
    params.shrinkCoeff = config_->shrinkCoeff;
    params.growCoeff = config_->growCoeff;
    params.maxShrinkSteps = config_->maxShrinkSteps;

    solver.reset(new steam::DoglegGaussNewtonSolver(problem.get(), params));
  } else if (config_->solver_type == "VanillaGaussNewton") {
    steam::VanillaGaussNewtonSolver::Params params;
    params.verbose = config_->verbose;
    params.maxIterations = config_->iterations;
    params.absoluteCostThreshold = config_->absoluteCostThreshold;
    params.absoluteCostChangeThreshold = config_->absoluteCostChangeThreshold;
    params.relativeCostChangeThreshold = config_->relativeCostChangeThreshold;
    solver.reset(new steam::VanillaGaussNewtonSolver(problem.get(), params));
  } else {
    LOG(ERROR) << "Unknown solver type: " << config_->solver_type;
  }
  return solver;
}

bool SteamModule::forceLM(
    std::shared_ptr<steam::OptimizationProblem> &problem) {
  try {
    backup_lm_solver_.reset(
        new steam::LevMarqGaussNewtonSolver(problem.get(), backup_params_));
    backup_lm_solver_->optimize();
  } catch (std::runtime_error &re) {
    LOG(ERROR) << "Back up LM failed, abandon hope....";
    return false;
  }
  backup_lm_solver_used_ = true;
  return true;
}

void SteamModule::runImpl(QueryCache &qdata, MapCache &mdata,
                          const Graph::ConstPtr &graph) {
  *qdata.steam_failure = false;
  backup_lm_solver_used_ = false;

  // basic sanity check
  if (!qdata.rig_features.is_valid() ||
      (qdata.success.is_valid() && *qdata.success == false)) {
    return;
  }

  /// \todo yuchen find a better place for this, or the following transformation
  /// code.
  if (!verifyInputData(qdata, mdata)) return;

  // Construct a transform evaluator that takes points from the vehicle frame
  // into the sensor frame.
  if (qdata.T_sensor_vehicle.is_valid()) {
    tf_sensor_vehicle_ = steam::se3::FixedTransformEvaluator::MakeShared(
        *qdata.T_sensor_vehicle);
  } else {
    tf_sensor_vehicle_ = steam::se3::FixedTransformEvaluator::MakeShared(
        lgmath::se3::Transformation());
  }

  for (auto it = qdata.T_sensor_vehicle_map->begin();
       it != qdata.T_sensor_vehicle_map->end(); ++it) {
    tf_sensor_vehicle_map_[it->first] =
        steam::se3::FixedTransformEvaluator::MakeShared(it->second);
  }

  tf_identity_ = steam::se3::FixedTransformEvaluator::MakeShared(
      lgmath::se3::Transformation());

  std::shared_ptr<steam::OptimizationProblem> problem;
  try {
    // PROBLEM SPECIFIC
    if (!verifyInputData(qdata, mdata)) return;
    problem = generateOptimizationProblem(qdata, mdata, graph);
    if (problem == nullptr) {
      LOG(ERROR) << "Couldn't generate optimization problem!" << std::endl;
      *qdata.steam_failure = true;
      return;
    }

    solver_ = generateSolver(problem);
    auto &steam_mutex = *qdata.steam_mutex;

    // default to success
    bool success = true;

    // attempt to run the solver
    try {
      std::lock_guard<std::mutex> iteration_lock(*steam_mutex.get());
      if (!config_->disable_solver) solver_->optimize();
    } catch (std::logic_error &e) {
      LOG(ERROR) << "Forced Gradient-Descent, running in LM..." << e.what();
      std::lock_guard<std::mutex> iteration_lock(*steam_mutex.get());
      success = forceLM(problem);
      *qdata.steam_failure = !success;
      *qdata.success = success;
    } catch (steam::unsuccessful_step &e) {
      // did any successful steps occur?
      if (solver_->getCurrIteration() <= 1) {
        // no: something is very wrong
        LOG(ERROR)
            << "Steam has failed to optimise the problem. This is an error";
        success = false;
        *qdata.steam_failure = !success;
        *qdata.success = success;
      } else {
        // yes: just a marginal problem, let's use what we got
        LOG(WARNING) << "Steam has failed due to an unsuccessful step. This "
                        "should be OK if it happens infrequently.";
      }
    } catch (std::runtime_error &e) {
      LOG(ERROR) << "Steam has failed with an unusual error: " << e.what();
      success = false;
      *qdata.steam_failure = !success;
      *qdata.success = success;
    }

    success = success && verifyOutputData(qdata, mdata);
    if (success == true) updateCaches(qdata, mdata);

  } catch (...) {
    *qdata.steam_failure = true;
    LOG(ERROR) << " bailing on steam problem!";
    *qdata.success = false;
  }

  if (config_->use_T_q_m_prior && qdata.T_r_m_prior.is_valid() == true &&
      (*qdata.T_r_m_prior).covarianceSet() && (*qdata.T_r_m).covarianceSet()) {
    double prior_ct_sigma = sqrt((*qdata.T_r_m_prior).cov()(1, 1));
    double ct_sigma = sqrt((*qdata.T_r_m).cov()(1, 1));
    if (ct_sigma > prior_ct_sigma) {
      LOG(WARNING) << "Loc. added uncertainty, bailing.";
      *qdata.success = false;
      *qdata.steam_failure = true;
    }
  }
}

MonoCalibPtr SteamModule::toMonoSteamCalibration(
    const vision::RigCalibration &calibration) {
  MonoCalibPtr sharedMonoIntrinsics(
      new vtr::steam_extensions::mono::CameraIntrinsics);
  sharedMonoIntrinsics->fu = calibration.intrinsics[0](0, 0);
  sharedMonoIntrinsics->fv = calibration.intrinsics[0](1, 1);
  sharedMonoIntrinsics->cu = calibration.intrinsics[0](0, 2);
  sharedMonoIntrinsics->cv = calibration.intrinsics[0](1, 2);
  return sharedMonoIntrinsics;
}

StereoCalibPtr SteamModule::toStereoSteamCalibration(
    const vision::RigCalibration &calibration) {
  StereoCalibPtr sharedStereoIntrinsics(new steam::stereo::CameraIntrinsics);
  sharedStereoIntrinsics->b = -calibration.extrinsics[1].matrix()(0, 3);
  sharedStereoIntrinsics->fu = calibration.intrinsics[0](0, 0);
  sharedStereoIntrinsics->fv = calibration.intrinsics[0](1, 1);
  sharedStereoIntrinsics->cu = calibration.intrinsics[0](0, 2);
  sharedStereoIntrinsics->cv = calibration.intrinsics[0](1, 2);
  return sharedStereoIntrinsics;
}

}  // namespace tactic
}  // namespace vtr
