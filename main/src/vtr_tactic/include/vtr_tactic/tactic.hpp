#pragma once

#include "rclcpp/rclcpp.hpp"

/// for visualization in ROS
#include <tf2/convert.h>
#include <tf2_eigen/tf2_eigen.h>
#include <tf2_ros/transform_broadcaster.h>
#include <nav_msgs/msg/odometry.hpp>
#include <nav_msgs/msg/path.hpp>

#include <cpo_interfaces/srv/query_trajectory.hpp>
#include <vtr_path_tracker/base.h>
#include <vtr_tactic/caches.hpp>
#include <vtr_tactic/memory_manager/live_memory_manager.hpp>
#include <vtr_tactic/memory_manager/map_memory_manager.hpp>
#include <vtr_tactic/pipelines/base_pipeline.hpp>
#include <vtr_tactic/publisher_interface.hpp>
#include <vtr_tactic/types.hpp>

using OdometryMsg = nav_msgs::msg::Odometry;
using ROSPathMsg = nav_msgs::msg::Path;
using PoseStampedMsg = geometry_msgs::msg::PoseStamped;
using TimeStampMsg = vtr_messages::msg::TimeStamp;
using QueryTrajectory = cpo_interfaces::srv::QueryTrajectory;

/// \todo define PathTracker::Ptr in Base
using PathTrackerPtr = std::shared_ptr<vtr::path_tracker::Base>;

namespace vtr {
namespace tactic {

using namespace vtr::mission_planning;

class Tactic : public mission_planning::StateMachineInterface {
 public:
  using Ptr = std::shared_ptr<Tactic>;

  using ChainLockType = std::lock_guard<std::recursive_mutex>;

  struct Config {
    using Ptr = std::shared_ptr<Config>;
    /** \brief Configuration for the localization chain */
    LocalizationChain::Config chain_config;
    /** \brief Configuration for the live memory manager */
    LiveMemoryManager::Config live_mem_config;
    /** \brief Configuration for the map memory manager */
    MapMemoryManager::Config map_mem_config;

    /** \brief Whether to extrapolate using STEAM trajectory for path tracker */
    bool extrapolate_odometry = false;
    /** \brief Default localization covariance when chain is not localized. */
    Eigen::Matrix<double, 6, 6> default_loc_cov;
    /** \brief Threshold for merging <x, y, theta> */
    std::vector<double> merge_threshold = {0.5, 0.25, 0.2};

    /**
     * \brief Whether to call the pipeline visualization functions and publish
     * odometry and localization to ROS.
     */
    bool visualize = false;

    static const Ptr fromROS(const rclcpp::Node::SharedPtr node);
  };

  Tactic(Config::Ptr config, const rclcpp::Node::SharedPtr node,
         BasePipeline::Ptr pipeline, Graph::Ptr graph)
      : node_(node),
        config_(config),
        graph_(graph),
        pipeline_(pipeline),
        chain_(config->chain_config, graph),
        live_mem_(config->live_mem_config, this, graph),
        map_mem_(config->map_mem_config, chain_mutex_ptr_, chain_, graph) {
    /// pipeline specific initialization
    pipeline_->initialize(graph_);

    /// start the memory managers
    live_mem_.start();
    map_mem_.start();

    query_gps_client_ = node_->create_client<QueryTrajectory>("query_trajectory");

    /// for visualization only
    tf_broadcaster_ = std::make_shared<tf2_ros::TransformBroadcaster>(node_);
    odo_path_pub_ = node_->create_publisher<ROSPathMsg>("odo_path", 10);
    loc_path_pub_ = node_->create_publisher<ROSPathMsg>("loc_path", 10);
  }

  virtual ~Tactic() {
    // wait for the pipeline to clear
    auto lck = lockPipeline();
    // stop the path tracker
    if (path_tracker_) path_tracker_->stopAndJoin();
  }

  void setPublisher(PublisherInterface* pub) { publisher_ = pub; }
  /** \brief Set the path tracker. */
  void setPathTracker(PathTrackerPtr pt) { path_tracker_ = pt; }

  LockType lockPipeline() {
    /// Lock so that no more data are passed into the pipeline
    LockType pipeline_lck(pipeline_mutex_);

    /// Pause the trackers/controllers but keep the current goal
    path_tracker::State pt_state;
    if (path_tracker_ && path_tracker_->isRunning()) {
      pt_state = path_tracker_->getState();
      if (pt_state != path_tracker::State::STOP) {
        LOG(INFO) << "pausing path tracker thread";
      }
      path_tracker_->pause();
    }

    /// Waiting for unfinished pipeline job
    if (pipeline_thread_future_.valid()) pipeline_thread_future_.get();

    /// Lock so taht no more data are passed into localization (during follow)
    std::lock_guard<std::mutex> loc_lck(loc_in_follow_mutex_);
    /// Waiting for unfinished localization job
    if (loc_in_follow_thread_future_.valid())
      loc_in_follow_thread_future_.get();

    /// \todo Waiting for unfinished jobs in pipeline
    pipeline_->waitForKeyframeJob();

    /// resume the trackers/controllers to their original state
    if (path_tracker_ && path_tracker_->isRunning()) {
      path_tracker_->setState(pt_state);
      LOG(INFO) << "resuming path tracker thread to state " << int(pt_state);
    }

    return pipeline_lck;
  }

  void setPipeline(const PipelineMode& pipeline_mode) override {
    LOG(DEBUG) << "[Lock Requested] setPipeline";
    auto lck = lockPipeline();
    LOG(DEBUG) << "[Lock Acquired] setPipeline";

    pipeline_mode_ = pipeline_mode;

    LOG(DEBUG) << "[Lock Released] setPipeline";
  };
  void runPipeline(QueryCache::Ptr qdata);

  void addRun(bool ephemeral = false, bool extend = false,
              bool save = true) override {
    /// \todo the following parameter are still needed?
    (void)ephemeral;
    (void)extend;
    (void)save;

    LOG(DEBUG) << "[Lock Requested] addRun";
    auto lck = lockPipeline();
    LOG(DEBUG) << "[Lock Acquired] addRun";

    /// \todo (yuchen) robot id is 0 for now
    graph_->addRun(0);

    // re-initialize the run
    first_frame_ = true;
    current_vertex_id_ = VertexId((uint64_t)-1);

    // re-initialize the pose records for visualization
    T_w_m_odo_ = lgmath::se3::TransformationWithCovariance(true);
    T_w_m_loc_ = lgmath::se3::TransformationWithCovariance(true);
    keyframe_poses_.clear();
    odometry_poses_.clear();

    LOG(DEBUG) << "[Lock Released] addRun";
  }

 public:
  /// \todo a bunch of functions to be implemented for state machine
  const Localization& persistentLoc() const { return persistent_loc_; }
  const Localization& targetLoc() const { return target_loc_; }
  void setTrunk(const VertexId& v) override {
    LOG(DEBUG) << "[Lock Requested] setTrunk";
    auto lck = lockPipeline();
    LOG(DEBUG) << "[Lock Acquired] setTrunk";

    persistent_loc_ = Localization(v);
    target_loc_ = Localization();

    if (publisher_) publisher_->publishRobot(persistent_loc_);

    LOG(DEBUG) << "[Lock Released] setTrunk";
  }
  double distanceToSeqId(const uint64_t& seq_id) {
    LockType lck(pipeline_mutex_);

    /// Clip the sequence ID to the max/min for the chain
    auto clip_seq = unsigned(std::min(seq_id, chain_.sequence().size() - 1));
    clip_seq = std::max(clip_seq, 0u);
    auto trunk_seq = unsigned(chain_.trunkSequenceId());

    /// if sequence is the same then we have arrived at this vertex.
    if (clip_seq == trunk_seq) return 0;

    ///
    unsigned start_seq = std::min(clip_seq, trunk_seq);
    unsigned end_seq = std::max(clip_seq, trunk_seq);
    // Compound raw distance along the path
    double dist = 0.;
    for (unsigned idx = start_seq; idx < end_seq; ++idx) {
      dist +=
          (chain_.pose(idx) * chain_.pose(idx + 1).inverse()).r_ab_inb().norm();
    }
    // Returns a negative value if we have passed that sequence already
    return (clip_seq < chain_.trunkSequenceId()) ? -dist : dist;
  }
  TacticStatus status() const { return status_; }
  LocalizationStatus tfStatus(const pose_graph::RCEdge::TransformType&) const {
    return LocalizationStatus::Forced;
  }

  const VertexId& closestVertexID() const override {
    return chain_.trunkVertexId();
  }

  const VertexId& currentVertexID() const override {
    return current_vertex_id_;
  }

  bool canCloseLoop() const override {
    std::string reason = "";
    /// Check persistent localization
    if (!persistent_loc_.localized)
      reason += "cannot localize against the live vertex; ";
    /// \todo check covariance

    /// Check target localization
    if (!target_loc_.localized)
      reason += "cannot localize against the target vertex for merging; ";
    /// \todo check covariance
    else {
      // Offset in the x, y, and yaw directions
      auto& T = target_loc_.T;
      double dx = T.r_ba_ina()(0), dy = T.r_ba_ina()(1), dt = T.vec()(5);
      if (dx > config_->merge_threshold[0] ||
          dy > config_->merge_threshold[1] ||
          dt > config_->merge_threshold[2]) {
        reason += "offset from path is too large to merge; ";
        LOG(WARNING) << "Offset from path is too large to merge (x, y, th): "
                     << dx << ", " << dy << " " << dt;
      }
    }
    if (!reason.empty()) {
      LOG(WARNING) << "Cannot merge because " << reason;
      return false;
    }
    return true;
  }

  void connectToTrunk(bool privileged, bool merge) override {
    LOG(DEBUG) << "[Lock Requested] connectToTrunk";
    auto lck = lockPipeline();
    LOG(DEBUG) << "[Lock Acquired] connectToTrunk";

    if (merge) {  /// For merging, i.e. loop closure
      LOG(INFO) << "Adding closure " << current_vertex_id_ << " --> "
                << chain_.trunkVertexId();
      LOG(DEBUG) << "with transform:\n" << chain_.T_petiole_trunk().inverse();
      graph_->addEdge(current_vertex_id_, chain_.trunkVertexId(),
                      chain_.T_petiole_trunk().inverse(), pose_graph::Spatial,
                      privileged);
    } else {  /// Upon successful metric localization.
      auto neighbours = graph_->at(current_vertex_id_)->spatialNeighbours();
      if (neighbours.size() == 0) {
        LOG(INFO) << "Adding connection " << current_vertex_id_ << " --> "
                  << chain_.trunkVertexId() << ", privileged: " << privileged;
        LOG(DEBUG) << "with transform:\n" << chain_.T_petiole_trunk().inverse();
        graph_->addEdge(current_vertex_id_, chain_.trunkVertexId(),
                        chain_.T_petiole_trunk().inverse(), pose_graph::Spatial,
                        privileged);
      } else if (neighbours.size() == 1) {
        LOG(INFO) << "Connection exists: " << current_vertex_id_ << " --> "
                  << *neighbours.begin()
                  << ", privileged set to: " << privileged;
        if (*neighbours.begin() != chain_.trunkVertexId()) {
          std::string err{"Not adding an edge connecting to trunk."};
          LOG(ERROR) << err;
          throw std::runtime_error{err};
        }
        graph_->at(current_vertex_id_, *neighbours.begin())
            ->setManual(privileged);
      } else {
        std::string err{"Should never reach here."};
        LOG(ERROR) << err;
        throw std::runtime_error{err};
      }
    }

    LOG(DEBUG) << "[Lock Released] connectToTrunk";
  }

  void relaxGraph() { graph_->callbacks()->updateRelaxation(steam_mutex_ptr_); }
  void saveGraph() {
    LOG(DEBUG) << "[Lock Requested] saveGraph";
    auto lck = lockPipeline();
    LOG(DEBUG) << "[Lock Acquired] saveGraph";
    graph_->save();
    LOG(DEBUG) << "[Lock Released] saveGraph";
  }

  void setPath(const VertexId::Vector& path, bool follow = false) {
    LOG(DEBUG) << "[Lock Requested] setPath";
    auto lck = lockPipeline();
    LOG(DEBUG) << "[Lock Acquired] setPath";

    /// Clear any existing path in UI
    if (publisher_) publisher_->clearPath();

    /// Set path and target localization
    /// \todo is it possible to put target loc into chain?
    LOG(INFO) << "Set path of size " << path.size();

    ChainLockType chain_lck(*chain_mutex_ptr_);
    chain_.setSequence(path);
    target_loc_ = Localization();

    if (path.size() > 0) {
      chain_.expand();
      publishPath(node_->now());
      if (follow && publisher_) {
        publisher_->publishPath(chain_);
        startPathTracker(chain_);
      }
    } else {
      // make sure path tracker is stopped
      stopPathTracker();
    }

    LOG(DEBUG) << "[Lock Released] setPath";
  }

  VertexId closest_ = VertexId::Invalid();
  VertexId current_ = VertexId::Invalid();
  TacticStatus status_;
  Localization loc_;

 public:
  /// Internal data query functions for debugging
  const std::vector<lgmath::se3::TransformationWithCovariance>& odometryPoses()
      const {
    return odometry_poses_;
  }

 private:
  void addConnectedVertex(
      const TimeStampMsg& stamp,
      const lgmath::se3::TransformationWithCovariance& T_r_m) {
    /// Add the new vertex
    auto previous_vertex_id = current_vertex_id_;
    addDanglingVertex(stamp);

    /// Add connection
    bool manual = (pipeline_mode_ == PipelineMode::Branching) ||
                  (pipeline_mode_ == PipelineMode::Merging);
    (void)graph_->addEdge(previous_vertex_id, current_vertex_id_, T_r_m,
                          pose_graph::Temporal, manual);
  }
  void addDanglingVertex(const TimeStampMsg& stamp) {
    /// Add the new vertex
    auto vertex = graph_->addVertex(stamp);
    current_vertex_id_ = vertex->id();
  }

  void updatePersistentLoc(const VertexId& v, const EdgeTransform& T,
                           bool localized) {
    // reset localization successes when switching to a new vertex
    if (persistent_loc_.v != v) persistent_loc_.successes = 0;
    persistent_loc_.v = v;
    persistent_loc_.T = T;
    persistent_loc_.localized = localized;

    if (localized && !T.covarianceSet()) {
      LOG(WARNING) << "Attempted to set target loc without a covariance!";
    }
  }

  void updateTargetLoc(const VertexId& v, const EdgeTransform& T,
                       bool localized) {
    // reset localization successes when switching to a new vertex
    if (target_loc_.v != v) target_loc_.successes = 0;
    target_loc_.v = v;
    target_loc_.T = T;
    target_loc_.localized = localized;

    if (localized && !T.covarianceSet()) {
      LOG(WARNING) << "Attempted to set target loc without a covariance!";
    }
  }

  void addGpsEdge(QueryCache::Ptr &qdata) {
    auto v_id1 = chain_.petioleVertexId();
    auto v_id2 = *qdata->live_id;
    auto e_id = EdgeId(v_id1, v_id2, pose_graph::Temporal);
    if (graph_->contains(e_id)) {
      LOG(INFO) << "Trying to set GPS transform for edge " << e_id;

      auto t_1 = graph_->at(v_id1)->keyFrameTime().nanoseconds_since_epoch;
      auto t_2 = graph_->at(v_id2)->keyFrameTime().nanoseconds_since_epoch;

      // wait for the service
      // todo: is this the proper behaviour? want it to not interfere with navigation
      while (!query_gps_client_->wait_for_service(50ms)) {
        if (!rclcpp::ok()) {
          LOG(ERROR) << "Interrupted while waiting for the service. Exiting.";
          return;
        }
        LOG(INFO) << "Query trajectory not available so not adding edge.";
        return;
      }

      // send and wait for the result
      auto request = std::make_shared<QueryTrajectory::Request>();
      request->t_1 = t_1;
      request->t_2 = t_2;

      auto response = query_gps_client_->async_send_request(request);

      if (rclcpp::spin_until_future_complete(node_, response) ==
          rclcpp::FutureReturnCode::SUCCESS) {
        LOG(INFO) << "Message back: " << response.get()->message;
        if (response.get()->success) {
          Eigen::Affine3d T_21_eig;
          Eigen::fromMsg(response.get()->tf_2_1.pose,T_21_eig);   // not sure if this is best way to convert. Seems to flip

          auto T_21 = lgmath::se3::TransformationWithCovariance(T_21_eig.matrix());
          // todo - also set covariance
          auto e = graph_->at(e_id);
          e->setTransformGps(T_21);
          LOG(INFO) << "Set gps edge to \n" << e->TGps().matrix();  //debug
          LOG(INFO) << "Vision edge is \n" << e->T().matrix();  //debug
        }
      } else {
        LOG(WARNING) << "Failed to call GPS service.";
      }
    } else {
      LOG(INFO) << "Couldn't find " << e_id << " in graph.";
    }
  }

 private:
  void startPathTracker(LocalizationChain& chain) {
    if (!path_tracker_) {
      LOG(WARNING) << "Path tracker not set! Cannot start control loop.";
      return;
    }

    LOG(INFO) << "Starting path tracker.";
    path_tracker_->followPathAsync(path_tracker::State::PAUSE, chain);
  }

  void stopPathTracker() {
    if (!path_tracker_) {
      LOG(WARNING) << "Path tracker not set! Cannot stop control loop.";
      return;
    }

    LOG(INFO) << "Stopping path tracker.";
    path_tracker_->stopAndJoin();
  }

 private:
  /** \brief Start running the pipeline (probably in a separate thread) */
  void runPipeline_(QueryCache::Ptr qdata);

  /**
   * \brief Runs localization job in path following (probably in a separate
   * thread)
   */
  void runLocalizationInFollow_(QueryCache::Ptr qdata);

  void updatePathTracker(QueryCache::Ptr qdata);

  void branch(QueryCache::Ptr qdata);
  void merge(QueryCache::Ptr qdata);
  void search(QueryCache::Ptr qdata);
  void follow(QueryCache::Ptr qdata);

  /** \brief Publishes odometry estimate in a global frame for visualization. */
  void publishOdometry(QueryCache::Ptr qdata);
  /** \brief Publishes the repeat path in a global frame for visualization. */
  void publishPath(rclcpp::Time rcl_stamp);
  /** \brief Publishes current frame localized against for visualization. */
  void publishLocalization(QueryCache::Ptr qdata);

 private:
  const rclcpp::Node::SharedPtr node_;

  Config::Ptr config_;
  PublisherInterface* publisher_;
  Graph::Ptr graph_;
  PathTrackerPtr path_tracker_;

  PipelineMode pipeline_mode_;
  BasePipeline::Ptr pipeline_;

  std::shared_ptr<std::recursive_mutex> chain_mutex_ptr_ =
      std::make_shared<std::recursive_mutex>();
  LocalizationChain chain_;

  LiveMemoryManager live_mem_;
  MapMemoryManager map_mem_;

  std::recursive_timed_mutex pipeline_mutex_;
  std::future<void> pipeline_thread_future_;

  std::mutex loc_in_follow_mutex_;
  std::future<void> loc_in_follow_thread_future_;

  bool first_frame_ = true;

  /** \brief STEAM is not thread safe, so need a mutex.*/
  std::shared_ptr<std::mutex> steam_mutex_ptr_ = std::make_shared<std::mutex>();

  /** \brief Vertex id of the latest keyframe, initialized to invalid */
  VertexId current_vertex_id_ = VertexId((uint64_t)-1);

  /** \brief Localization against the map, that persists across runs. */
  Localization persistent_loc_;
  /** \brief Localization against a target for merging. */
  Localization target_loc_;

  /** \brief Transformation from the latest keyframe to world frame */
  lgmath::se3::TransformationWithCovariance T_w_m_odo_ =
      lgmath::se3::TransformationWithCovariance(true);
  /** \brief Transformation from the localization keyframe to world frame */
  lgmath::se3::TransformationWithCovariance T_w_m_loc_ =
      lgmath::se3::TransformationWithCovariance(true);
  std::vector<PoseStampedMsg> keyframe_poses_;
  std::vector<lgmath::se3::TransformationWithCovariance> odometry_poses_;

  std::shared_ptr<tf2_ros::TransformBroadcaster> tf_broadcaster_;
  rclcpp::Publisher<ROSPathMsg>::SharedPtr odo_path_pub_;
  rclcpp::Publisher<ROSPathMsg>::SharedPtr loc_path_pub_;

  /** \brief Client to call query GPS trajectory service */
  rclcpp::Client<QueryTrajectory>::SharedPtr query_gps_client_;
};

}  // namespace tactic
}  // namespace vtr