#include <vtr_tactic/tactic.hpp>

namespace vtr {
namespace tactic {

auto Tactic::Config::fromROS(const rclcpp::Node::SharedPtr node) -> const Ptr {
  auto config = std::make_shared<Config>();
  // clang-format off
  /// setup localization chain
  config->chain_config.min_cusp_distance = node->declare_parameter<double>("tactic.chain.min_cusp_distance", 1.5);
  config->chain_config.angle_weight = node->declare_parameter<double>("tactic.chain.angle_weight", 7.0);
  config->chain_config.search_depth = node->declare_parameter<int>("tactic.chain.search_depth", 20);
  config->chain_config.search_back_depth = node->declare_parameter<int>("tactic.chain.search_back_depth", 10);
  config->chain_config.distance_warning = node->declare_parameter<double>("tactic.chain.distance_warning", 3);

  /// setup live memory manager
  config->live_mem_config.enable = node->declare_parameter<bool>("tactic.live_mem.enable", true);
  config->live_mem_config.lookahead_distance = (unsigned)node->declare_parameter<int>("tactic.live_mem.lookahead_distance", 15);
  config->live_mem_config.window_size = (unsigned)node->declare_parameter<int>("tactic.live_mem.window_size", 250);

  /// setup live memory manager
  config->map_mem_config.enable = node->declare_parameter<bool>("tactic.map_mem.enable", true);
  config->map_mem_config.lookahead_distance = node->declare_parameter<int>("tactic.map_mem.lookahead_distance", 15);
  config->map_mem_config.vertex_life_span = node->declare_parameter<int>("tactic.map_mem.vertex_life_span", 10);
  config->map_mem_config.priv_streams_to_load = node->declare_parameter<std::vector<std::string>>("tactic.map_mem.priv_streams_to_load", std::vector<std::string>());
  config->map_mem_config.streams_to_load = node->declare_parameter<std::vector<std::string>>("tactic.map_mem.streams_to_load", std::vector<std::string>());

  /// setup tactic
  config->extrapolate_odometry = node->declare_parameter<bool>("tactic.extrapolate_odometry", false);
  auto dlc = node->declare_parameter<std::vector<double>>("tactic.default_loc_cov", std::vector<double>{});
  if (dlc.size() != 6) {
    CLOG(WARNING, "tactic") << "Tactic default localization covariance malformed ("
                 << dlc.size() << " elements). Must be 6 elements!";
  }
  // make at least size elements to prevent segfault
  if (dlc.size() < 6) dlc.resize(6, 1.);
  config->default_loc_cov.setZero();
  config->default_loc_cov.diagonal() << dlc[0], dlc[1], dlc[2], dlc[3], dlc[4], dlc[5];

  config->merge_threshold = node->declare_parameter<std::vector<double>>("tactic.merge_threshold", std::vector<double>{0.5, 0.25, 0.2});

  config->visualize = node->declare_parameter<bool>("tactic.visualize", false);
  // clang-format on
  return config;
}

void Tactic::runPipeline(QueryCache::Ptr qdata) {
  /// Lock to make sure the pipeline does not change during processing
  LockType lck(pipeline_mutex_, std::defer_lock_t());
  // If we cannot lock in 30ms, give up and hope that the next frame will work
  if (!lck.try_lock_for(std::chrono::milliseconds(30))) {
    CLOG(WARNING, "tactic")
        << "Dropping frame due to unavailable pipeline mutex";
    return;
  }

  /// Setup caches
  // Some pipelines use rviz for visualization, give them the ros node for
  // creating publishers.
  qdata->node = node_;
  // \todo Steam has been made thread safe for VTR, remove this and any use of
  // it.
  qdata->steam_mutex.fallback(steam_mutex_ptr_);

  /// Preprocess incoming data, which always runs no matter what mode we are in.
  CLOG(DEBUG, "tactic") << "Preprocessing incoming data.";
  pipeline_->preprocess(qdata, graph_);
  if (config_->visualize) pipeline_->visualizePreprocess(qdata, graph_);

#ifdef DETERMINISTIC_VTR
  CLOG(DEBUG, "tactic") << "Finished preprocessing incoming data.";
  runPipeline_(qdata);
#else
  /// Run pipeline according to the state
  if (pipeline_thread_future_.valid()) pipeline_thread_future_.get();
  CLOG(DEBUG, "tactic") << "Launching the odometry and mapping thread.";
  pipeline_thread_future_ = std::async(std::launch::async, [this, qdata]() {
    el::Helpers::setThreadName("tactic.odometry_mapping");
    runPipeline_(qdata);
  });
  CLOG(DEBUG, "tactic") << "Finished preprocessing incoming data.";
#endif
}

void Tactic::runPipeline_(QueryCache::Ptr qdata) {
  CLOG(DEBUG, "tactic") << "Running odometry and mapping on incoming data.";

  /// Setup caches
  /// \todo Move the following to somewhere better like a dedicated odometry
  /// function.
  qdata->first_frame.fallback(first_frame_);
  qdata->live_id.fallback(current_vertex_id_);
  qdata->keyframe_test_result.fallback(KeyframeTestResult::DO_NOTHING);
  qdata->odo_success.fallback(true);

  switch (pipeline_mode_) {
    case PipelineMode::Idle:
      break;
    /// \note There are lots of repetitive code in the following four functions,
    /// maybe we can combine them at some point, but for now, consider leaving
    /// them separate so that it is slightly more clear what is happening during
    /// each pipeline mode.
    case PipelineMode::Branching:
      branch(qdata);
      break;
    case PipelineMode::Merging:
      merge(qdata);
      break;
    case PipelineMode::Searching:
      search(qdata);
      break;
    case PipelineMode::Following:
      follow(qdata);
      break;
  }

  first_frame_ = false;

  CLOG(DEBUG, "tactic")
      << "Finish running odometry and mapping on incoming data.";
}

void Tactic::branch(QueryCache::Ptr qdata) {
  /// Prior assumes no motion since last processed frame
  qdata->T_r_m_odo.fallback(chain_.T_leaf_petiole());

  CLOG(DEBUG, "tactic") << "Prior transformation from live vertex ("
                        << *qdata->live_id
                        << ") to robot (i.e., T_r_m odometry): "
                        << *qdata->T_r_m_odo;

  /// Odometry to get relative pose estimate and whether a keyframe should be
  /// created
  pipeline_->runOdometry(qdata, graph_);
  // add to a global odometry estimate vector for debugging
  odometry_poses_.push_back(T_w_m_odo_ * (*qdata->T_r_m_odo).inverse());
  if (config_->visualize) {
    publishOdometry(qdata);
    pipeline_->visualizeOdometry(qdata, graph_);
  }

  CLOG(DEBUG, "tactic") << "Estimated transformation from live vertex ("
                        << *qdata->live_id
                        << ") to robot (i.e., T_r_m odometry): "
                        << *qdata->T_r_m_odo;

  {
    CLOG(DEBUG, "tactic") << "[ChainLock Requested] branch";
    ChainLockType lck(*chain_mutex_ptr_);
    CLOG(DEBUG, "tactic") << "[ChainLock Acquired] branch";

    /// Update Odometry in localization chain
    chain_.updatePetioleToLeafTransform(*qdata->T_r_m_odo, true);

    /// Update persistent localization (only when the first keyframe has been
    /// created so that current vertex id is valid.)
    if (current_vertex_id_.isValid())
      updatePersistentLoc(*qdata->live_id, *qdata->T_r_m_odo, true);

    /// Update the localization with respect to the privileged chain.
    /// \todo this is never true in branch mode
    updateTargetLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                    chain_.isLocalized());

    /// Publish odometry result on live robot localization
    if (publisher_)
      publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                               target_loc_, qdata->rcl_stamp.ptr());

    CLOG(DEBUG, "tactic") << "[ChainLock Released] branch";
  }

  /// Check if we should create a new vertex
  const auto &keyframe_test_result = *(qdata->keyframe_test_result);
  if (keyframe_test_result == KeyframeTestResult::CREATE_VERTEX) {
    /// Add new vertex to the posegraph
    bool first_keyframe = !current_vertex_id_.isValid();
    if (!current_vertex_id_.isValid())
      addDanglingVertex(*(qdata->stamp));
    else
      addConnectedVertex(*(qdata->stamp), *(qdata->T_r_m_odo));
    CLOG(INFO, "tactic") << "Creating a new keyframe with id "
                         << current_vertex_id_;

    /// Update live id to the just-created vertex id
    qdata->live_id.fallback(current_vertex_id_);

    /// Call the pipeline to process the keyframe
    pipeline_->processKeyframe(qdata, graph_, current_vertex_id_);

    /// Compute odometry in world frame for visualization.
    T_w_m_odo_ = T_w_m_odo_ * (*qdata->T_r_m_odo).inverse();
    keyframe_poses_.emplace_back();
    keyframe_poses_.back().pose =
        tf2::toMsg(Eigen::Affine3d(T_w_m_odo_.matrix()));

    /// We try to branch off from existing experience, so the first frame
    /// connects to existing graph based on persistent localization
    /// \todo This block should be moved to metric localization state in teach
    if (first_keyframe && persistent_loc_.v.isSet()) {
      /// Add target id for localization and prior
      qdata->map_id.fallback(persistent_loc_.v);
      qdata->T_r_m_loc.fallback(persistent_loc_.T);
      /// \todo Localization success not used currently. Need the metric
      /// localization pipeline running before branch
      *qdata->loc_success = false;

      CLOG(INFO, "tactic") << "Branching from existing experience: "
                           << persistent_loc_.v << " --> "
                           << current_vertex_id_;
      pipeline_->runLocalization(qdata, graph_);

      CLOG(DEBUG, "tactic")
          << "Estimated transformation from trunk to robot (T_r_m "
             "localization): "
          << *qdata->T_r_m_loc;

      CLOG(INFO, "tactic") << "Adding new branch with offset: "
                           << (*qdata->T_r_m_loc).inverse().vec().transpose();
      (void)graph_->addEdge(current_vertex_id_, persistent_loc_.v,
                            (*qdata->T_r_m_loc).inverse(), pose_graph::Spatial,
                            true);
    }

    {
      /// Update Odometry in localization chain
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] branch";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] branch";

      /// Must happen in key frame
      chain_.setPetiole(current_vertex_id_);
      chain_.updatePetioleToLeafTransform(EdgeTransform(true), true);

      /// Reset the map to robot transform and new vertex flag
      updatePersistentLoc(current_vertex_id_, EdgeTransform(true), true);
      /// Update odometry result on live robot localization
      if (publisher_)
        publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                                 target_loc_);

      CLOG(DEBUG, "tactic") << "[ChainLock Released] branch";
    }
  }
}

void Tactic::follow(QueryCache::Ptr qdata) {
  /// Prior assumes no motion since last processed frame
  qdata->T_r_m_odo.fallback(chain_.T_leaf_petiole());

  CLOG(DEBUG, "tactic") << "Prior transformation from live vertex ("
                        << *qdata->live_id
                        << ") to robot (i.e., T_r_m odometry): "
                        << *qdata->T_r_m_odo;

  /// Odometry to get relative pose estimate and whether a keyframe should be
  /// created
  pipeline_->runOdometry(qdata, graph_);
  // add to a global odometry estimate vector for debugging
  odometry_poses_.push_back(T_w_m_odo_ * (*qdata->T_r_m_odo).inverse());
  if (config_->visualize) {
    publishOdometry(qdata);
    pipeline_->visualizeOdometry(qdata, graph_);
  }

  CLOG(DEBUG, "tactic") << "Estimated transformation from live vertex ("
                        << *qdata->live_id
                        << ") to robot (i.e., T_r_m odometry): "
                        << *qdata->T_r_m_odo;

  {
    CLOG(DEBUG, "tactic") << "[ChainLock Requested] follow";
    ChainLockType lck(*chain_mutex_ptr_);
    CLOG(DEBUG, "tactic") << "[ChainLock Acquired] follow";

    /// Update Odometry in localization chain,
    /// Never search backwards in path following.
    chain_.updatePetioleToLeafTransform(*qdata->T_r_m_odo, false);

    /// Update the localization with respect to the privileged chain.
    updatePersistentLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                        chain_.isLocalized());

    /// Publish odometry result on live robot localization
    if (publisher_)
      publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                               target_loc_);

    /// Send localization updates to path tracker
    updatePathTracker(qdata);

    CLOG(DEBUG, "tactic") << "[ChainLock Released] follow";
  }

  /// Check if we should create a new vertex
  const auto &keyframe_test_result = *(qdata->keyframe_test_result);
  if (keyframe_test_result == KeyframeTestResult::CREATE_VERTEX) {
    /// Add new vertex to the posegraph
    bool first_keyframe = !current_vertex_id_.isValid();
    if (!current_vertex_id_.isValid())
      addDanglingVertex(*(qdata->stamp));
    else
      addConnectedVertex(*(qdata->stamp), *(qdata->T_r_m_odo));
    (void)first_keyframe;
    CLOG(INFO, "tactic") << "Creating a new keyframe with id "
                         << current_vertex_id_;

    /// Update live id to the just-created vertex id
    qdata->live_id.fallback(current_vertex_id_);

    /// Call the pipeline to process the keyframe
    pipeline_->processKeyframe(qdata, graph_, current_vertex_id_);
#ifdef DETERMINISTIC_VTR
    {
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] follow";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] follow";

      /// Must happen in key frame
      chain_.setPetiole(current_vertex_id_);
      chain_.updatePetioleToLeafTransform(EdgeTransform(true), false);

      /// Compute odometry and localization in world frame for visualization
      T_w_m_odo_ = chain_.T_start_leaf();
      keyframe_poses_.emplace_back();
      keyframe_poses_.back().pose =
          tf2::toMsg(Eigen::Affine3d(T_w_m_odo_.matrix()));
      T_w_m_loc_ = chain_.T_start_trunk();

      /// Add target vertex for localization and prior
      qdata->map_id.fallback(chain_.trunkVertexId());
      if (!chain_.isLocalized()) {
        qdata->T_r_m_loc.fallback(
            Eigen::Matrix4d(Eigen::Matrix4d::Identity(4, 4)));
        const Eigen::Matrix<double, 6, 6> loc_cov = config_->default_loc_cov;
        qdata->T_r_m_loc->setCovariance(loc_cov);
      } else {
        qdata->T_r_m_loc.fallback(chain_.T_petiole_trunk());
      }
      *qdata->loc_success = false;

      CLOG(DEBUG, "tactic") << "[ChainLock Released] follow";
    }

    // Run the localizer against the closest vertex
    pipeline_->runLocalization(qdata, graph_);
    if (config_->visualize) {
      publishLocalization(qdata);
      pipeline_->visualizeLocalization(qdata, graph_);
    }

    CLOG(DEBUG, "tactic") << "Estimated transformation from trunk vertex ("
                          << *(qdata->map_id) << ") to petiole vertex ("
                          << *(qdata->live_id)
                          << ") (i.e., T_r_m localization): "
                          << *qdata->T_r_m_loc;

    /// Add an edge no matter localization is successful or not
    /// \todo this might not be necessary when not running localization in
    /// parallel
    const auto &T_r_m_loc = *(qdata->T_r_m_loc);

    // update the transform
    auto edge_id =
        EdgeId(*(qdata->live_id), *(qdata->map_id), pose_graph::Spatial);
    if (graph_->contains(edge_id)) {
      graph_->at(edge_id)->setTransform(T_r_m_loc.inverse());
    } else {
      CLOG(DEBUG, "tactic")
          << "Adding a spatial edge between " << *(qdata->live_id) << " and "
          << *(qdata->map_id) << " to the graph.";
      graph_->addEdge(*(qdata->live_id), *(qdata->map_id), T_r_m_loc.inverse(),
                      pose_graph::Spatial, false);
      CLOG(DEBUG, "tactic")
          << "Done adding the spatial edge between " << *(qdata->live_id)
          << " and " << *(qdata->map_id) << " to the graph.";
    }

    {
      /// Update Odometry in localization chain
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] follow";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] follow";

      /// Move the localization chain forward upon successful localization
      chain_.convertPetioleTrunkToTwigBranch();
      chain_.updateBranchToTwigTransform(T_r_m_loc, false);

      /// Update the localization with respect to the privileged chain
      updatePersistentLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                          chain_.isLocalized());

      /// Publish odometry result on live robot localization
      if (publisher_)
        publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                                 target_loc_);

      CLOG(DEBUG, "tactic") << "[ChainLock Released] follow";
    }

#else
    /// Lock so that no more data are passed into localization (during follow)
    CLOG(DEBUG, "tactic") << "[LocalizationLock Requested] follow";
    std::lock_guard<std::mutex> loc_lck(loc_in_follow_mutex_);
    CLOG(DEBUG, "tactic") << "[LocalizationLock Acquired] follow";
    /// Waiting for unfinished localization job
    if (loc_in_follow_thread_future_.valid())
      loc_in_follow_thread_future_.get();

    {
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] follow";
      ChainLockType chain_lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] follow";

      /// Must happen in key frame
      chain_.setPetiole(current_vertex_id_);
      chain_.updatePetioleToLeafTransform(EdgeTransform(true), false);

      /// Compute odometry and localization in world frame for visualization
      T_w_m_odo_ = chain_.T_start_leaf();
      keyframe_poses_.emplace_back();
      keyframe_poses_.back().pose =
          tf2::toMsg(Eigen::Affine3d(T_w_m_odo_.matrix()));
      T_w_m_loc_ = chain_.T_start_trunk();

      /// Add target vertex for localization and prior
      qdata->map_id.fallback(chain_.trunkVertexId());
      if (!chain_.isLocalized()) {
        qdata->T_r_m_loc.fallback(
            Eigen::Matrix4d(Eigen::Matrix4d::Identity(4, 4)));
        const Eigen::Matrix<double, 6, 6> loc_cov = config_->default_loc_cov;
        qdata->T_r_m_loc->setCovariance(loc_cov);
      } else {
        qdata->T_r_m_loc.fallback(chain_.T_petiole_trunk());
      }
      *qdata->loc_success = false;

      CLOG(DEBUG, "tactic") << "[ChainLock Released] follow";
    }

    // Run the localizer against the closest vertex
    // pipeline_->runLocalization(qdata, graph_);
    if (config_->visualize) {
      publishLocalization(qdata);
      // pipeline_->visualizeLocalization(qdata, graph_);
    }

    // CLOG(DEBUG, "tactic") << "Estimated transformation from trunk vertex ("
    //            << *(qdata->map_id) << ") to petiole vertex ("
    //            << *(qdata->live_id)
    //            << ") (i.e., T_r_m localization): " << *qdata->T_r_m_loc;

    CLOG(DEBUG, "tactic") << "Prior transformation from trunk vertex ("
                          << *(qdata->map_id) << ") to petiole vertex ("
                          << *(qdata->live_id)
                          << ") (i.e., T_r_m localization): "
                          << *qdata->T_r_m_loc;

    /// Add an edge no matter localization is successful or not
    const auto &T_r_m_loc = *(qdata->T_r_m_loc);

    // update the transform
    auto edge_id =
        EdgeId(*(qdata->live_id), *(qdata->map_id), pose_graph::Spatial);
    if (graph_->contains(edge_id)) {
      graph_->at(edge_id)->setTransform(T_r_m_loc.inverse());
    } else {
      CLOG(DEBUG, "tactic")
          << "Adding a spatial edge between " << *(qdata->live_id) << " and "
          << *(qdata->map_id) << " to the graph.";
      graph_->addEdge(*(qdata->live_id), *(qdata->map_id), T_r_m_loc.inverse(),
                      pose_graph::Spatial, false);
      CLOG(DEBUG, "tactic") << "Done adding the spatial edge between "
                            << *(qdata->live_id) << " and " << *(qdata->map_id)
                            << " to the graph. T_from_to (T_qm): " << T_r_m_loc;
    }

    {
      /// Update Odometry in localization chain
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] follow";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] follow";

      /// Move the localization chain forward upon successful localization
      chain_.convertPetioleTrunkToTwigBranch();
      chain_.updateBranchToTwigTransform(T_r_m_loc, false);

      /// Update the localization with respect to the privileged chain
      updatePersistentLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                          chain_.isLocalized());

      /// Publish odometry result on live robot localization
      if (publisher_)
        publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                                 target_loc_);

      CLOG(DEBUG, "tactic") << "[ChainLock Released] follow";
    }

    CLOG(DEBUG, "tactic") << "Launching the localization in follow thread.";
    loc_in_follow_thread_future_ =
        std::async(std::launch::async, [this, qdata]() {
          el::Helpers::setThreadName("tactic.localization");
          runLocalizationInFollow_(qdata);
        });
    CLOG(DEBUG, "tactic") << "[LocalizationLock Released] follow";
#endif
  }
}

void Tactic::runLocalizationInFollow_(QueryCache::Ptr qdata) {
  CLOG(DEBUG, "tactic") << "Start running localization in follow.";

  // Run the localizer against the closest vertex
  pipeline_->runLocalization(qdata, graph_);
  if (config_->visualize) {
    // publishLocalization(qdata);
    pipeline_->visualizeLocalization(qdata, graph_);
  }

  CLOG(DEBUG, "tactic") << "Estimated transformation from trunk vertex ("
                        << *(qdata->map_id) << ") to petiole vertex ("
                        << *(qdata->live_id) << ") (i.e., T_r_m localization): "
                        << *qdata->T_r_m_loc;

  /// T_r_m_loc is assumed not changed if localization pipeline failed, so in
  /// that case the following code essentially does nothing.
  const auto &T_r_m_loc = *(qdata->T_r_m_loc);

  /// Update the transform
  auto edge_id =
      EdgeId(*(qdata->live_id), *(qdata->map_id), pose_graph::Spatial);
  if (!graph_->contains(edge_id)) {
    /// Currently assumes that an edge is inserted upon every keyframe during
    /// following
    std::string error{"Edge required by localization does not exist!"};
    CLOG(ERROR, "tactic") << error;
    throw std::runtime_error{error};
  }
  graph_->at(edge_id)->setTransform(T_r_m_loc.inverse());

  {
    CLOG(DEBUG, "tactic") << "[ChainLock Requested] follow";
    ChainLockType lck(*chain_mutex_ptr_);
    CLOG(DEBUG, "tactic") << "[ChainLock Acquired] follow";

    /// Update the transform
    chain_.updateBranchToTwigTransform(T_r_m_loc, false);

    /// Update the localization with respect to the privileged chain
    updatePersistentLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                        chain_.isLocalized());
    /// Publish odometry result on live robot localization
    if (publisher_)
      publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                               target_loc_);

    /// Update keyframe_poses_ (guaranteed to update the most recent
    /// keyframe_pose)
    keyframe_poses_.back().pose =
        tf2::toMsg(Eigen::Affine3d(chain_.T_start_leaf().matrix()));

    CLOG(DEBUG, "tactic") << "[ChainLock Released] follow";
  }

  CLOG(DEBUG, "tactic") << "Finish running localization in follow.";
}

void Tactic::updatePathTracker(QueryCache::Ptr qdata) {
  if (!path_tracker_) {
    CLOG(WARNING, "tactic")
        << "Path tracker not set, skip updating path tracker.";
    return;
  }

  CLOG(DEBUG, "tactic") << "[ChainLock Requested] updatePathTracker";
  ChainLockType lck(*chain_mutex_ptr_);
  CLOG(DEBUG, "tactic") << "[ChainLock Acquired] updatePathTracker";

  LOG(DEBUG) << "trunk vid: " << chain_.trunkVertexId()
             << " branch vid: " << chain_.branchVertexId()
             << " twig vid: " << chain_.twigVertexId()
             << " pet vid: " << chain_.petioleVertexId();

  // We need to know where we are to update the path tracker.,,
  if (!chain_.isLocalized()) {
    CLOG(WARNING, "tactic")
        << "Chain isn't localized; delaying localization update to "
           "path tracker.";
    return;
  }

  /// \todo this is the same as stamp, not sure why defined again
  uint64_t im_stamp_ns = (*qdata->stamp).nanoseconds_since_epoch;
  if (config_->extrapolate_odometry && qdata->trajectory.is_valid()) {
    // Send an update to the path tracker including the trajectory
    path_tracker_->notifyNewLeaf(chain_, *qdata->trajectory, current_vertex_id_,
                                 im_stamp_ns);
  } else {
    // Update the transform in the new path tracker if we did not use STEAM
    path_tracker_->notifyNewLeaf(chain_, common::timing::toChrono(im_stamp_ns),
                                 current_vertex_id_);
  }

  CLOG(DEBUG, "tactic") << "[ChainLock Released] updatePathTracker";
}

void Tactic::merge(QueryCache::Ptr qdata) {
  /// Prior assumes no motion since last processed frame
  qdata->T_r_m_odo.fallback(chain_.T_leaf_petiole());

  CLOG(DEBUG, "tactic") << "Prior transformation from live vertex ("
                        << *qdata->live_id
                        << ") to robot (i.e., T_r_m odometry): "
                        << *qdata->T_r_m_odo;

  /// Odometry to get relative pose estimate and whether a keyframe should be
  /// created
  pipeline_->runOdometry(qdata, graph_);
  // add to a global odometry estimate vector for debugging
  odometry_poses_.push_back(T_w_m_odo_ * (*qdata->T_r_m_odo).inverse());
  if (config_->visualize) {
    publishOdometry(qdata);
    pipeline_->visualizeOdometry(qdata, graph_);
  }

  CLOG(DEBUG, "tactic") << "Estimated transformation from live vertex ("
                        << *qdata->live_id
                        << ") to robot (i.e., T_r_m odometry): "
                        << *qdata->T_r_m_odo;

  {
    CLOG(DEBUG, "tactic") << "[ChainLock Requested] merge";
    ChainLockType lck(*chain_mutex_ptr_);
    CLOG(DEBUG, "tactic") << "[ChainLock Acquired] merge";

    /// Update Odometry in localization chain
    chain_.updatePetioleToLeafTransform(*qdata->T_r_m_odo, true);

    /// Update persistent localization (only when the first keyframe has been
    /// created so that current vertex id is valid.)
    if (current_vertex_id_.isValid())
      updatePersistentLoc(*qdata->live_id, *qdata->T_r_m_odo, true);

    /// Update the localization with respect to the privileged chain
    /// (target localization)
    updateTargetLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                    chain_.isLocalized(), false);

    /// Publish odometry result on live robot localization
    if (publisher_)
      publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                               target_loc_);

    CLOG(DEBUG, "tactic") << "[ChainLock Released] merge";
  }

  /// \note Old merge pipeline may increase keyframe creation frequency to
  /// localize more frequently (when not localized). but now we simply run
  /// localization on every frame - by doing this we assume that the robot
  /// moves slowly during merge.

  /// Check if we should create a new vertex
  const auto &keyframe_test_result = *(qdata->keyframe_test_result);
  if (keyframe_test_result == KeyframeTestResult::CREATE_VERTEX) {
    CLOG(INFO, "tactic") << "Create a new keyframe and localize against the "
                            "merge path!";

    /// Add new vertex to the posegraph
    bool first_keyframe = !current_vertex_id_.isValid();
    if (!current_vertex_id_.isValid())
      addDanglingVertex(*(qdata->stamp));
    else
      addConnectedVertex(*(qdata->stamp), *(qdata->T_r_m_odo));
    CLOG(INFO, "tactic") << "Creating a new keyframe with id "
                         << current_vertex_id_;

    /// Update live id to the just-created vertex id
    qdata->live_id.fallback(current_vertex_id_);

    /// Call the pipeline to make and process the keyframe
    pipeline_->processKeyframe(qdata, graph_, current_vertex_id_);

    /// Compute odometry in world frame for visualization.
    T_w_m_odo_ = T_w_m_odo_ * (*qdata->T_r_m_odo).inverse();
    keyframe_poses_.emplace_back();
    keyframe_poses_.back().pose =
        tf2::toMsg(Eigen::Affine3d(T_w_m_odo_.matrix()));

    /// \todo Should also localize against persistent localization (i.e.
    /// localize against trunk when branching from existing graph), but
    /// currently assume that we never run merge at the very beginning of a
    /// teach. A loop with 2 vertices? Please.
    if (first_keyframe && persistent_loc_.v.isSet()) {
      std::string err{
          "Started merging right after branching, this should never happen."};
      CLOG(ERROR, "tactic") << err;
      throw std::runtime_error{err};
    }

    {
      /// Update Odometry in localization chain
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] merge";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] merge";

      /// Must happen in key frame
      chain_.setPetiole(current_vertex_id_);
      chain_.updatePetioleToLeafTransform(EdgeTransform(true), true);

      /// Reset the map to robot transform and new vertex flag
      updatePersistentLoc(current_vertex_id_, EdgeTransform(true), true);
      /// Update odometry result on live robot localization
      if (publisher_)
        publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                                 target_loc_);

      /// Add target vertex for localization and prior
      qdata->map_id.fallback(chain_.trunkVertexId());
      if (!chain_.isLocalized()) {
        qdata->T_r_m_loc.fallback(
            Eigen::Matrix4d(Eigen::Matrix4d::Identity(4, 4)));
        const Eigen::Matrix<double, 6, 6> loc_cov = config_->default_loc_cov;
        qdata->T_r_m_loc->setCovariance(loc_cov);
      } else {
        qdata->T_r_m_loc.fallback(chain_.T_petiole_trunk());
      }
      *qdata->loc_success = false;

      CLOG(DEBUG, "tactic") << "[ChainLock Released] merge";
    }

    // Run the localizer against the closest vertex
    pipeline_->runLocalization(qdata, graph_);
    // if (config_->visualize) {
    //   publishLocalization(qdata);
    //   pipeline_->visualizeLocalization(qdata, graph_);
    // }

    if (*qdata->loc_success) {
      CLOG(DEBUG, "tactic")
          << "Estimated transformation from trunk vertex (" << *(qdata->map_id)
          << ") to petiole vertex (" << *(qdata->live_id)
          << ") (i.e., T_r_m localization): " << *qdata->T_r_m_loc;

      {
        CLOG(DEBUG, "tactic") << "[ChainLock Requested] merge";
        ChainLockType lck(*chain_mutex_ptr_);
        CLOG(DEBUG, "tactic") << "[ChainLock Acquired] merge";

        /// Move the localization chain forward upon successful localization
        chain_.convertPetioleTrunkToTwigBranch();
        chain_.updateBranchToTwigTransform(*qdata->T_r_m_loc, false);

        /// Update the localization with respect to the privileged chain
        /// (target localization)
        updateTargetLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                        chain_.isLocalized(), false);

        CLOG(DEBUG, "tactic") << "[ChainLock Released] merge";
      }

      /// Increment localization success
      target_loc_.successes++;
    } else {
      CLOG(INFO, "tactic")
          << "Failed to estimate transformation from localization vertex ("
          << *(qdata->map_id) << ") to live vertex (" << *(qdata->live_id)
          << ")";

      /// Decrement localization success
      if (target_loc_.successes > 0) target_loc_.successes = 0;
      target_loc_.successes--;
    }
  } else {
    CLOG(INFO, "tactic")
        << "Localize against the merge path without creating a new keyframe!";

    if (!current_vertex_id_.isValid()) {
      std::string err{
          "Started merging right after branching, this should never happen."};
      CLOG(ERROR, "tactic") << err;
      throw std::runtime_error{err};
    }

    {
      /// Setup localization cache
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] merge";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] merge";

      // current_vertex_id_ is basically chain_.petioleVertexId, but
      // petioleVertexId is not set if the first frame in merge is not a
      // keyframe.
      qdata->live_id.fallback(current_vertex_id_);
      qdata->map_id.fallback(chain_.trunkVertexId());
      if (!chain_.isLocalized()) {
        qdata->T_r_m_loc.fallback(
            Eigen::Matrix4d(Eigen::Matrix4d::Identity(4, 4)));
        const Eigen::Matrix<double, 6, 6> loc_cov = config_->default_loc_cov;
        qdata->T_r_m_loc->setCovariance(loc_cov);
      } else {
        /// we have to use T_leaf_trunk not T_petiole_trunk because
        /// petiole!=leaf in this case.
        qdata->T_r_m_loc.fallback(chain_.T_leaf_trunk());
      }
      *qdata->loc_success = false;

      CLOG(DEBUG, "tactic") << "[ChainLock Released] merge";
    }

    // Run the localizer against the closest vertex
    pipeline_->runLocalization(qdata, graph_);
    // if (config_->visualize) {
    //   publishLocalization(qdata);
    //   pipeline_->visualizeLocalization(qdata, graph_);
    // }

    if (*qdata->loc_success) {
      CLOG(DEBUG, "tactic")
          << "Estimated transformation from trunk vertex (" << *qdata->map_id
          << ") to petiole(+leaf) vertex (" << *qdata->live_id
          << ") (i.e., T_r_m localization): " << *qdata->T_r_m_loc;

      {
        CLOG(DEBUG, "tactic") << "[ChainLock Requested] merge";
        ChainLockType lck(*chain_mutex_ptr_);
        CLOG(DEBUG, "tactic") << "[ChainLock Acquired] merge";

        /// Move the localization chain forward upon successful localization
        chain_.convertPetioleTrunkToTwigBranch();
        const auto T_r_m_loc =
            (*qdata->T_r_m_odo).inverse() * (*qdata->T_r_m_loc);
        chain_.updateBranchToTwigTransform(T_r_m_loc, false);

        /// Update the localization with respect to the privileged chain
        /// (target localization)
        updateTargetLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                        chain_.isLocalized(), false);

        CLOG(DEBUG, "tactic") << "[ChainLock Released] merge";
      }

      /// Increment localization success
      target_loc_.successes++;
    } else {
      CLOG(INFO, "tactic")
          << "Failed to estimate transformation from localization vertex ("
          << *qdata->map_id << ") to live vertex (" << *qdata->live_id << ")";

      /// Decrement localization success
      if (target_loc_.successes > 0) target_loc_.successes = 0;
      target_loc_.successes--;
    }
  }

  CLOG(INFO, "tactic") << "Localizing againt vertex: " << target_loc_.v
                       << ", number of successes is " << target_loc_.successes;
  if (target_loc_.successes < -5) {
    /// \todo need better logic here to decide whether or not to move to the
    /// next vertex to localize against (maybe a binary search)
    CLOG(INFO, "tactic")
        << "Cannot localize against this vertex, move to the next one.";
    {
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] merge";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] merge";

      auto trunk_seq = (chain_.trunkSequenceId() + 3) %
                       uint32_t(chain_.sequence().size() - 1);
      chain_.resetTrunk(trunk_seq);

      CLOG(DEBUG, "tactic") << "[ChainLock Released] merge";
    }
  }

  if (config_->visualize) {
    publishLocalization(qdata);
    pipeline_->visualizeLocalization(qdata, graph_);
  }
}

void Tactic::search(QueryCache::Ptr qdata) {
  /// Prior assumes no motion since last processed frame
  qdata->T_r_m_odo.fallback(chain_.T_leaf_petiole());

  CLOG(DEBUG, "tactic") << "Prior transformation from live vertex ("
                        << *qdata->live_id
                        << ") to robot (i.e., T_r_m odometry): "
                        << *qdata->T_r_m_odo;

  /// Odometry to get relative pose estimate and whether a keyframe should be
  /// created
  pipeline_->runOdometry(qdata, graph_);
  // add to a global odometry estimate vector for debugging
  odometry_poses_.push_back(T_w_m_odo_ * (*qdata->T_r_m_odo).inverse());
  if (config_->visualize) {
    publishOdometry(qdata);
    pipeline_->visualizeOdometry(qdata, graph_);
  }

  CLOG(DEBUG, "tactic") << "Estimated transformation from live vertex ("
                        << *(qdata->live_id)
                        << ") to robot (i.e., T_r_m odometry): "
                        << *qdata->T_r_m_odo;

  {
    CLOG(DEBUG, "tactic") << "[ChainLock Requested] search";
    ChainLockType lck(*chain_mutex_ptr_);
    CLOG(DEBUG, "tactic") << "[ChainLock Acquired] search";

    /// Update Odometry in localization chain
    chain_.updatePetioleToLeafTransform(*qdata->T_r_m_odo, true);

    /// Update persistent localization (only when the first keyframe has been
    /// created so that current vertex id is valid.)
    // if (current_vertex_id_.isValid())
    //   updatePersistentLoc(*qdata->live_id, *qdata->T_r_m_odo, true);

    /// Update the localization with respect to the privileged chain
    /// (target localization)
    // updateTargetLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
    //                 chain_.isLocalized());

    /// Publish odometry result on live robot localization
    // if (publisher_)
    //   publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
    //                            target_loc_);

    CLOG(DEBUG, "tactic") << "[ChainLock Released] search";
  }

  /// \note Old merge pipeline may increase keyframe creation frequency to
  /// localize more frequently (when not localized). but now we simply run
  /// localization on every frame - by doing this we assume that the robot
  /// moves slowly during localization search.

  /// Check if we should create a new vertex
  const auto &keyframe_test_result = *(qdata->keyframe_test_result);
  if (keyframe_test_result == KeyframeTestResult::CREATE_VERTEX) {
    CLOG(INFO, "tactic") << "Create a new keyframe and localize against the "
                            "search vertices!";

    /// Add new vertex to the posegraph
    bool first_keyframe = !current_vertex_id_.isValid();
    if (!current_vertex_id_.isValid())
      addDanglingVertex(*(qdata->stamp));
    else
      addConnectedVertex(*(qdata->stamp), *(qdata->T_r_m_odo));
    (void)first_keyframe;
    CLOG(INFO, "tactic") << "Creating a new keyframe with id "
                         << current_vertex_id_;

    /// Update live id to the just-created vertex id
    qdata->live_id.fallback(current_vertex_id_);

    /// Call the pipeline to make and process the keyframe
    pipeline_->processKeyframe(qdata, graph_, current_vertex_id_);

    {
      /// Update Odometry in localization chain
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] search";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] search";

      /// Must happen in key frame
      chain_.setPetiole(current_vertex_id_);
      chain_.updatePetioleToLeafTransform(EdgeTransform(true), true);

      /// Reset the map to robot transform and new vertex flag
      // updatePersistentLoc(current_vertex_id_, EdgeTransform(true), true);
      /// Update odometry result on live robot localization
      // if (publisher_)
      //   publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
      //                            target_loc_);

      /// Add target vertex for localization and prior
      qdata->map_id.fallback(chain_.trunkVertexId());
      if (!chain_.isLocalized()) {
        qdata->T_r_m_loc.fallback(
            Eigen::Matrix4d(Eigen::Matrix4d::Identity(4, 4)));
        const Eigen::Matrix<double, 6, 6> loc_cov = config_->default_loc_cov;
        qdata->T_r_m_loc->setCovariance(loc_cov);
      } else {
        qdata->T_r_m_loc.fallback(chain_.T_petiole_trunk());
      }
      *qdata->loc_success = false;

      CLOG(DEBUG, "tactic") << "[ChainLock Released] search";
    }

    // Run the localizer against the closest vertex
    pipeline_->runLocalization(qdata, graph_);
    // if (config_->visualize) {
    //   publishLocalization(qdata);
    //   pipeline_->visualizeLocalization(qdata, graph_);
    // }

    if (*qdata->loc_success) {
      CLOG(DEBUG, "tactic")
          << "Estimated transformation from trunk vertex (" << *(qdata->map_id)
          << ") to petiole vertex (" << *(qdata->live_id)
          << ") (i.e., T_r_m localization): " << *qdata->T_r_m_loc;

      {
        CLOG(DEBUG, "tactic") << "[ChainLock Requested] search";
        ChainLockType lck(*chain_mutex_ptr_);
        CLOG(DEBUG, "tactic") << "[ChainLock Acquired] search";

        /// Move the localization chain forward upon successful localization
        chain_.convertPetioleTrunkToTwigBranch();
        chain_.updateBranchToTwigTransform(*(qdata->T_r_m_loc), false);

        // /// Update the localization with respect to the privileged chain
        // /// (target localization)
        // updateTargetLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
        //                 chain_.isLocalized());

        /// Update the localization with respect to the privileged chain
        /// (target localization)
        updatePersistentLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                            chain_.isLocalized(), false);

        /// Publish odometry result on live robot localization
        if (publisher_)
          publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                                   target_loc_);

        CLOG(DEBUG, "tactic") << "[ChainLock Released] search";
      }
      /// Increment localization success
      persistent_loc_.successes++;
    } else {
      CLOG(INFO, "tactic")
          << "Failed to estimate transformation from localization vertex ("
          << *(qdata->map_id) << ") to live vertex (" << *(qdata->live_id)
          << ")";

      /// Decrement localization success
      if (persistent_loc_.successes > 0) persistent_loc_.successes = 0;
      persistent_loc_.successes--;
    }
  } else {
    CLOG(INFO, "tactic")
        << "Localize against the search vertices without creating a "
           "new keyframe!";

    if (!current_vertex_id_.isValid()) {
      std::string err{
          "No keyframe created since the start of teach/repeat. If this is a "
          "repeat, then your pipeline must create a keyframe on first "
          "frame."};
      CLOG(ERROR, "tactic") << err;
      throw std::runtime_error{err};
    }

    {
      /// Setup localization cache
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] search";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] search";

      // Since the first frame must be a keyframe, we can assume that petiole is
      // set at this moment.
      /// \todo this assumption may not be true - first frame may not be a
      /// keyframe
      qdata->live_id.fallback(chain_.petioleVertexId());
      qdata->map_id.fallback(chain_.trunkVertexId());
      if (!chain_.isLocalized()) {
        qdata->T_r_m_loc.fallback(
            Eigen::Matrix4d(Eigen::Matrix4d::Identity(4, 4)));
        const Eigen::Matrix<double, 6, 6> loc_cov = config_->default_loc_cov;
        qdata->T_r_m_loc->setCovariance(loc_cov);
      } else {
        /// we have to use T_leaf_trunk not T_petiole_trunk because
        /// petiole!=leaf in this case.
        qdata->T_r_m_loc.fallback(chain_.T_leaf_trunk());
      }
      *qdata->loc_success = false;

      CLOG(DEBUG, "tactic") << "[ChainLock Released] search";
    }

    // Run the localizer against the closest vertex
    pipeline_->runLocalization(qdata, graph_);
    // if (config_->visualize) {
    //   publishLocalization(qdata);
    //   pipeline_->visualizeLocalization(qdata, graph_);
    // }

    if (*qdata->loc_success) {
      CLOG(DEBUG, "tactic")
          << "Estimated transformation from trunk vertex (" << *qdata->map_id
          << ") to petiole(+leaf) vertex (" << *qdata->live_id
          << ") (i.e., T_r_m localization): " << *qdata->T_r_m_loc;

      {
        CLOG(DEBUG, "tactic") << "[ChainLock Requested] search";
        ChainLockType lck(*chain_mutex_ptr_);
        CLOG(DEBUG, "tactic") << "[ChainLock Acquired] search";

        /// Move the localization chain forward upon successful localization
        chain_.convertPetioleTrunkToTwigBranch();
        const auto T_r_m_loc =
            (*qdata->T_r_m_odo).inverse() * (*qdata->T_r_m_loc);
        chain_.updateBranchToTwigTransform(T_r_m_loc, false);

        /// Update the localization with respect to the privileged chain
        /// (target localization)
        // updateTargetLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
        //                 chain_.isLocalized());

        /// Update the localization with respect to the privileged chain
        /// (target localization)
        updatePersistentLoc(chain_.trunkVertexId(), chain_.T_leaf_trunk(),
                            chain_.isLocalized(), false);

        /// Publish odometry result on live robot localization
        if (publisher_)
          publisher_->publishRobot(persistent_loc_, chain_.trunkSequenceId(),
                                   target_loc_);

        CLOG(DEBUG, "tactic") << "[ChainLock Released] search";
      }

      /// Increment localization success
      persistent_loc_.successes++;
    } else {
      CLOG(INFO, "tactic")
          << "Failed to estimate transformation from localization vertex ("
          << *qdata->map_id << ") to live vertex (" << *qdata->live_id << ")";

      /// Decrement localization success
      if (persistent_loc_.successes > 0) persistent_loc_.successes = 0;
      persistent_loc_.successes--;
    }
  }

  CLOG(INFO, "tactic") << "Localizing againt vertex: " << persistent_loc_.v
                       << ", number of successes is "
                       << persistent_loc_.successes;
  if (persistent_loc_.successes < -5) {
    /// \todo need better logic here to decide whether or not to move to the
    /// next vertex to localize against (maybe a binary search)
    CLOG(INFO, "tactic")
        << "Cannot localize against this vertex, move to the next one.";
    {
      CLOG(DEBUG, "tactic") << "[ChainLock Requested] search";
      ChainLockType lck(*chain_mutex_ptr_);
      CLOG(DEBUG, "tactic") << "[ChainLock Acquired] search";

      /// \todo for now, only search the first 30 vertices
      auto trunk_seq = (chain_.trunkSequenceId() + 3) % uint32_t(30);
      chain_.resetTrunk(trunk_seq);

      CLOG(DEBUG, "tactic") << "[ChainLock Released] search";
    }
  }

  if (config_->visualize) {
    publishLocalization(qdata);
    pipeline_->visualizeLocalization(qdata, graph_);
  }
}

void Tactic::publishOdometry(QueryCache::Ptr qdata) {
  /// Publish the latest keyframe estimate in world frame
  Eigen::Affine3d T(T_w_m_odo_.matrix());
  auto msg = tf2::eigenToTransform(T);
  msg.header.frame_id = "world";
  msg.header.stamp = *(qdata->rcl_stamp);
  msg.child_frame_id = "odometry keyframe";
  tf_broadcaster_->sendTransform(msg);

  /// Publish the teach path
  ROSPathMsg path;
  path.header.frame_id = "world";
  path.header.stamp = *(qdata->rcl_stamp);
  path.poses = keyframe_poses_;
  odo_path_pub_->publish(path);

  /// Publish the current frame
  Eigen::Affine3d T2(qdata->T_r_m_odo->inverse().matrix());
  auto msg2 = tf2::eigenToTransform(T2);
  msg2.header.frame_id = "odometry keyframe";
  msg2.header.stamp = *(qdata->rcl_stamp);
  msg2.child_frame_id = *(qdata->robot_frame);
  tf_broadcaster_->sendTransform(msg2);
  if (*(qdata->robot_frame) != "robot") {
    msg2.child_frame_id = "robot";
    tf_broadcaster_->sendTransform(msg2);
  }
}

void Tactic::publishPath(rclcpp::Time rcl_stamp) {
  std::vector<Eigen::Affine3d> eigen_poses;
  /// publish the repeat path in
  chain_.expand();
  for (unsigned i = 0; i < chain_.sequence().size(); i++) {
    eigen_poses.push_back(Eigen::Affine3d(chain_.pose(i).matrix()));
  }

  /// Publish the repeat path
  ROSPathMsg path;
  path.header.frame_id = "world";
  path.header.stamp = rcl_stamp;
  auto &poses = path.poses;
  for (const auto &pose : eigen_poses) {
    PoseStampedMsg ps;
    ps.pose = tf2::toMsg(pose);
    poses.push_back(ps);
  }
  loc_path_pub_->publish(path);
}

void Tactic::publishLocalization(QueryCache::Ptr qdata) {
  /// Publish the current frame localized against in world frame
  Eigen::Affine3d T(T_w_m_loc_.matrix());
  auto msg = tf2::eigenToTransform(T);
  msg.header.frame_id = "world";
  msg.header.stamp = *(qdata->rcl_stamp);
  // apply an offset to y axis to separate odometry and localization
  msg.transform.translation.z -= 10;
  msg.child_frame_id = "localization keyframe";
  tf_broadcaster_->sendTransform(msg);
}

}  // namespace tactic
}  // namespace vtr