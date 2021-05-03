#include <vtr_steam_extensions/evaluator/tdcp_error_eval.hpp>

#include <utility>

namespace steam {

TdcpErrorEval::TdcpErrorEval(const double phi_dd,
                             se3::PositionEvaluator::ConstPtr &r_ba_ina,
                             se3::TransformEvaluator::ConstPtr &T_ag,
                             Eigen::Vector3d r_1a_ing_ata,
                             Eigen::Vector3d r_1a_ing_atb,
                             Eigen::Vector3d r_2a_ing_ata,
                             Eigen::Vector3d r_2a_ing_atb)
    : phi_dd_(phi_dd),
      r_ba_ina_(r_ba_ina),
      T_ag_(T_ag),
      r_1a_ing_ata_(std::move(r_1a_ing_ata)),
      r_1a_ing_atb_(std::move(r_1a_ing_atb)),
      r_2a_ing_ata_(std::move(r_2a_ing_ata)),
      r_2a_ing_atb_(std::move(r_2a_ing_atb)),
      u_a21_((r_2a_ing_ata_ - r_1a_ing_ata_).normalized()) {
}

bool TdcpErrorEval::isActive() const {
  return r_ba_ina_->isActive() || T_ag_->isActive();
}

Eigen::Matrix<double, 1, 1> TdcpErrorEval::evaluate() const {

  EvalTreeHandle<lgmath::se3::Transformation> blkAutoEvalTransform = T_ag_->getBlockAutomaticEvaluation();
  const lgmath::se3::Transformation &T_ag = blkAutoEvalTransform.getValue();
  EvalTreeHandle<Eigen::Vector3d> blkAutoEvalPosition = r_ba_ina_->getBlockAutomaticEvaluation();
  const Eigen::Vector3d &r = blkAutoEvalPosition.getValue();

  const Eigen::Matrix3d &C_ag = T_ag.C_ba();
  double rho_1a = r_1a_ing_ata_.norm();
  double rho_2a = r_2a_ing_ata_.norm();
  double rho_1b = (r_1a_ing_atb_ - C_ag.transpose() * r).norm();
  double rho_2b = (r_2a_ing_atb_ - C_ag.transpose() * r).norm();

  double rho_dd = (rho_2b - rho_2a) - (rho_1b - rho_1a);
  double error = phi_dd_ - rho_dd;

  return Eigen::Matrix<double, 1, 1>{error};
}

Eigen::Matrix<double, 1, 1> TdcpErrorEval::evaluate(const Eigen::Matrix<double, 1, 1> &lhs,
                                                    std::vector<Jacobian<1, 6>> *jacs) const {
// Check and initialize Jacobian array
  if (jacs == nullptr) {
    throw std::invalid_argument("Null pointer provided to return-input 'jacs' in evaluate");
  }
  jacs->clear();

  // If current pose unlocked, add Jacobian from perturbing it
  if (r_ba_ina_->isActive()) {

    // Get evaluation tree
    EvalTreeHandle<Eigen::Vector3d> blkAutoEvalPosition = r_ba_ina_->getBlockAutomaticEvaluation();

    EvalTreeHandle<lgmath::se3::Transformation> blkAutoEvalTransform = T_ag_->getBlockAutomaticEvaluation();
    const lgmath::se3::Transformation &T_ag = blkAutoEvalTransform.getValue();

    // Get Jacobians
    Eigen::Matrix<double, 1, 3> J_1 = u_a21_.transpose() * T_ag.C_ba().transpose(); // u^T * C_ag^T

    Eigen::Matrix<double, 1, 3> newLhs = lhs * J_1;
    r_ba_ina_->appendBlockAutomaticJacobians(newLhs, blkAutoEvalPosition.getRoot(), jacs);
  }

  // If vehicle orientation is unlocked, add Jacobian from perturbing it
  if (T_ag_->isActive()) {
    // Get evaluation tree
    EvalTreeHandle<lgmath::se3::Transformation> blkAutoEvalTransform = T_ag_->getBlockAutomaticEvaluation();

    // Get evaluation from tree
    const lgmath::se3::Transformation &T_ag = blkAutoEvalTransform.getValue();

    // manual implementation of skew-symmetric operator
    EvalTreeHandle<Eigen::Vector3d> blkAutoEvalPosition = r_ba_ina_->getBlockAutomaticEvaluation();
    const Eigen::Vector3d &r = blkAutoEvalPosition.getValue();
    Eigen::Matrix3d r_hat;
    r_hat << 0, -r(2), r(1),
        r(2), 0, -r(0),
        -r(1), r(0), 0;

    // Get Jacobians
    Eigen::Matrix<double, 1, 3> J_2 = u_a21_.transpose() * T_ag.C_ba().transpose() * r_hat;

    Eigen::Matrix<double, 1, 6> newLhs;
    newLhs << 0, 0, 0, lhs * J_2;
    T_ag_->appendBlockAutomaticJacobians(newLhs, blkAutoEvalTransform.getRoot(), jacs);
  }

  return evaluate();
}

} // steam
