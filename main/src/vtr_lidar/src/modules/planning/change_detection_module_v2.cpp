// Copyright 2021, Autonomous Space Robotics Lab (ASRL)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/**
 * \file change_detection_module_v2.cpp
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#include "vtr_lidar/modules/planning/change_detection_module_v2.hpp"

#include "pcl/features/normal_3d.h"

#include "vtr_lidar/data_types/costmap.hpp"

namespace vtr {
namespace lidar {

namespace {

template <class PointT>
void computeCentroidAndNormal(const pcl::PointCloud<PointT> &points,
                              Eigen::Vector3f &centroid,
                              Eigen::Vector3f &normal, float &roughness) {
  // 16-bytes aligned placeholder for the XYZ centroid of a surface patch
  Eigen::Vector4f centroid_homo;
  // Placeholder for the 3x3 covariance matrix at each surface patch
  Eigen::Matrix3f cov;

  // Estimate the XYZ centroid
  pcl::compute3DCentroid(points, centroid_homo);

  // Compute the 3x3 covariance matrix
  pcl::computeCovarianceMatrix(points, centroid_homo, cov);

  // Compute pca
  Eigen::SelfAdjointEigenSolver<Eigen::Matrix3f> es;
  es.compute(cov);

  // dump results
  centroid = centroid_homo.head<3>();
  normal = es.eigenvectors().col(0);
  roughness = std::sqrt(es.eigenvalues()(0));  // standard deviation
}

#if false
template <typename PointT>
class DetectChangeOp {
 public:
  DetectChangeOp(const pcl::PointCloud<PointT> &points,
                 const std::vector<float> &distances,
                 const std::vector<float> &roughnesses,
                 const float &search_radius)
      : points_(points),
        distances_(distances),
        roughnesses_(roughnesses),
        sq_search_radius_(search_radius * search_radius),
        adapter_(points) {
    /// create kd-tree of the point cloud for radius search
    kdtree_ = std::make_unique<KDTree<PointT>>(2, adapter_,
                                               KDTreeParams(/* max leaf */ 10));
    kdtree_->buildIndex();
    // search params setup
    search_params_.sorted = false;
  }

  void operator()(const Eigen::Vector2f &point, float &value) const {
    /// find the nearest neighbors
    std::vector<std::pair<size_t, float>> inds_dists;
    size_t num_neighbors = kdtree_->radiusSearch(
        point.data(), sq_search_radius_, inds_dists, search_params_);

    if (num_neighbors < 1) {
#if false
      CLOG(WARNING, "lidar.change_detection")
          << "looking at point: <" << x << "," << y << ">, roughness: 0"
          << " (no enough neighbors)";
#endif
      value = 10.0;  /// \todo arbitrary high value
      return;
    }
    std::vector<float> distances;
    std::vector<float> roughnesses;
    std::vector<float> neg_logprobs;
    distances.reserve(num_neighbors);
    roughnesses.reserve(num_neighbors);
    neg_logprobs.reserve(num_neighbors);
    for (size_t i = 0; i < num_neighbors; ++i) {
      distances.emplace_back(distances_[inds_dists[i].first]);
      roughnesses.emplace_back(std::sqrt(roughnesses_[inds_dists[i].first]));

      const auto &dist = distances_[inds_dists[i].first];
      const auto &rough = roughnesses_[inds_dists[i].first];
      // std::pow(dist, 2) / rough / 2 + std::log(std::sqrt(rough))
      neg_logprobs.emplace_back(std::pow(dist, 2) / (rough + 0.01) / 2.0);
    }
    // CLOG(DEBUG, "lidar.change_detection")
    //     << "\n Distance is " << distances << "\n roughness is " <<
    //     roughnesses;
    // use the negative log probability as the cost
    // value = *std::max_element(neg_logprobs.begin(), neg_logprobs.end());
    value = neg_logprobs[(size_t)std::floor((float)neg_logprobs.size() / 2)];
  }

 private:
  /** \brief reference to the point cloud */
  const pcl::PointCloud<PointT> &points_;
  const std::vector<float> &distances_;
  const std::vector<float> &roughnesses_;

  /** \brief squared search radius */
  const float sq_search_radius_;

  KDTreeSearchParams search_params_;
  NanoFLANNAdapter<PointT> adapter_;
  std::unique_ptr<KDTree<PointT>> kdtree_;
};
#endif
}  // namespace

using namespace tactic;

auto ChangeDetectionModuleV2::Config::fromROS(
    const rclcpp::Node::SharedPtr &node, const std::string &param_prefix)
    -> ConstPtr {
  auto config = std::make_shared<Config>();
  // clang-format off
  // change detection
  config->search_radius = node->declare_parameter<float>(param_prefix + ".search_radius", config->search_radius);
  // cost map
  config->resolution = node->declare_parameter<float>(param_prefix + ".resolution", config->resolution);
  config->size_x = node->declare_parameter<float>(param_prefix + ".size_x", config->size_x);
  config->size_y = node->declare_parameter<float>(param_prefix + ".size_y", config->size_y);
  // general
  config->run_online = node->declare_parameter<bool>(param_prefix + ".run_online", config->run_online);
  config->run_async = node->declare_parameter<bool>(param_prefix + ".run_async", config->run_async);
  config->visualize = node->declare_parameter<bool>(param_prefix + ".visualize", config->visualize);
  // clang-format on
  return config;
}

void ChangeDetectionModuleV2::run_(QueryCache &qdata0, OutputCache &output0,
                                   const Graph::Ptr &graph,
                                   const TaskExecutor::Ptr &executor) {
  auto &qdata = dynamic_cast<LidarQueryCache &>(qdata0);
  // auto &output = dynamic_cast<LidarOutputCache &>(output0);

  const auto &vid_loc = *qdata.vid_loc;

  if (config_->run_async)
    executor->dispatch(std::make_shared<Task>(
        shared_from_this(), qdata0.shared_from_this(), 0, Task::DepIdSet{},
        Task::DepId{}, "Change Detection", vid_loc));
  else
    runAsync_(qdata0, output0, graph, executor, Task::Priority(-1),
              Task::DepId());
}

void ChangeDetectionModuleV2::runAsync_(
    QueryCache &qdata0, OutputCache &output0, const Graph::Ptr &,
    const TaskExecutor::Ptr &, const Task::Priority &, const Task::DepId &) {
  auto &qdata = dynamic_cast<LidarQueryCache &>(qdata0);
  auto &output = dynamic_cast<LidarOutputCache &>(output0);

  // visualization setup
  if (config_->visualize) {
    std::unique_lock<std::mutex> lock(mutex_);
    if (!publisher_initialized_) {
      // clang-format off
      tf_bc_ = std::make_shared<tf2_ros::TransformBroadcaster>(*qdata.node);
      scan_pub_ = qdata.node->create_publisher<PointCloudMsg>("change_detection_scan", 5);
      map_pub_ = qdata.node->create_publisher<PointCloudMsg>("change_detection_map", 5);
      costmap_pub_ = qdata.node->create_publisher<OccupancyGridMsg>("change_detection_costmap", 5);
      pointcloud_pub_ = qdata.node->create_publisher<PointCloudMsg>("change_detection_pointcloud", 5);
      // clang-format on
      publisher_initialized_ = true;
    }
  }

  if (config_->run_online &&
      output.chain->trunkSequenceId() != *qdata.sid_loc) {
    CLOG(INFO, "lidar.change_detection")
        << "Trunk id has changed, skip change detection for this scan";
    return;
  }

  // inputs
  const auto &stamp = *qdata.stamp;
  const auto &T_s_r = *qdata.T_s_r;
  const auto &vid_loc = *qdata.vid_loc;
  const auto &sid_loc = *qdata.sid_loc;
  const auto &T_r_v_loc = *qdata.T_r_v_loc;
  const auto &query_points = *qdata.undistorted_point_cloud;
  const auto &submap_loc = *qdata.submap_loc;
  const auto &map_point_cloud = submap_loc.point_cloud();
  const auto &T_v_m_loc = *qdata.T_v_m_loc;

  CLOG(INFO, "lidar.change_detection")
      << "Change detection for lidar scan at stamp: " << stamp;

  // clang-format off
  // Eigen matrix of original data (only shallow copy of ref clouds)
  const auto query_mat = query_points.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::cartesian_offset());
  const auto query_norms_mat = query_points.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::normal_offset());

  // retrieve the pre-processed scan and convert it to the localization frame
  pcl::PointCloud<PointWithInfo> aligned_points(query_points);
  auto aligned_mat = aligned_points.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::cartesian_offset());
  auto aligned_norms_mat = aligned_points.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::normal_offset());

  const auto T_m_s = (T_s_r * T_r_v_loc * T_v_m_loc).inverse().matrix();
  Eigen::Matrix3f C_m_s = (T_m_s.block<3, 3>(0, 0)).cast<float>();
  Eigen::Vector3f r_s_m_in_m = (T_m_s.block<3, 1>(0, 3)).cast<float>();
  aligned_mat = (C_m_s * query_mat).colwise() + r_s_m_in_m;
  aligned_norms_mat = C_m_s * query_norms_mat;
  // clang-format on

  // create kd-tree of the map
  NanoFLANNAdapter<PointWithInfo> adapter(map_point_cloud);
  KDTreeSearchParams search_params;
  KDTreeParams tree_params(10 /* max leaf */);
  auto kdtree =
      std::make_unique<KDTree<PointWithInfo>>(3, adapter, tree_params);
  kdtree->buildIndex();

  std::vector<long unsigned> nn_inds(aligned_points.size());
  std::vector<float> nn_dists(aligned_points.size());
  // compute nearest neighbors
  for (size_t i = 0; i < aligned_points.size(); i++) {
    KDTreeResultSet result_set(1);
    result_set.init(&nn_inds[i], &nn_dists[i]);
    kdtree->findNeighbors(result_set, aligned_points[i].data, search_params);
  }
  // compute planar distance
  const auto sq_search_radius = config_->search_radius * config_->search_radius;
  std::vector<float> roughnesses(aligned_points.size(), 0.0f);
  for (size_t i = 0; i < aligned_points.size(); i++) {
    // filter based on point to point distance  /// \todo parameters
    if (nn_dists[i] > 1.0f) {
      nn_dists[i] = -1;
      continue;
    }

    // radius search of the closest point
    std::vector<std::pair<size_t, float>> inds_dists;
    kdtree->radiusSearch(map_point_cloud[nn_inds[i]].data, sq_search_radius,
                         inds_dists, search_params);
    std::vector<int> indices(inds_dists.size());
    for (const auto &ind_dist : inds_dists) indices.push_back(ind_dist.first);

    // filter based on neighbors in map /// \todo parameters
    if (indices.size() < 10) {
      nn_dists[i] = -1;
      continue;
    }

    // compute the planar distance
    Eigen::Vector3f centroid, normal;
    // float roughness;
    computeCentroidAndNormal(
        pcl::PointCloud<PointWithInfo>(map_point_cloud, indices), centroid,
        normal, roughnesses[i]);

    const auto diff = aligned_points[i].getVector3fMap() - centroid;
    nn_dists[i] = std::abs(diff.dot(normal));
  }

  for (size_t i = 0; i < aligned_points.size(); i++) {
    aligned_points[i].normal_variance = 1.0f;
    if (nn_dists[i] < 0)
      aligned_points[i].normal_variance = 0.0f;
    else if (nn_dists[i] > roughnesses[i])
      aligned_points[i].normal_variance = 0.0f;
  }

  // clang-format off
  // retrieve the pre-processed scan and convert it to the robot frame
  pcl::PointCloud<PointWithInfo> aligned_points2(aligned_points);
  auto aligned_mat2 = aligned_points2.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::cartesian_offset());
  auto aligned_norms_mat2 = aligned_points2.getMatrixXfMap(3, PointWithInfo::size(), PointWithInfo::normal_offset());

  const auto T_r_m = (T_m_s * T_s_r).inverse().matrix();
  Eigen::Matrix3f C_r_m = (T_r_m.block<3, 3>(0, 0)).cast<float>();
  Eigen::Vector3f r_m_r_in_r = (T_r_m.block<3, 1>(0, 3)).cast<float>();
  aligned_mat2 = (C_r_m * aligned_mat).colwise() + r_m_r_in_r;
  aligned_norms_mat2 = C_r_m * aligned_norms_mat;
  // clang-format on

#if false
  // project to 2d and construct the grid map
  const auto costmap = std::make_shared<DenseCostMap>(
      config_->resolution, config_->size_x, config_->size_y);
  // update cost map based on change detection result
  DetectChangeOp<PointWithInfo> detect_change_op(
      aligned_points2, nn_dists, nn_roughnesses, config_->search_radius);
  costmap->update(detect_change_op);
  // add transform to the localization vertex
  costmap->T_vertex_this() = T_r_v_loc.inverse();
  costmap->vertex_id() = vid_loc;
  costmap->vertex_sid() = sid_loc;
#endif

  /// publish the transformed pointcloud
  if (config_->visualize) {
    std::unique_lock<std::mutex> lock(mutex_);
    //
    const auto T_w_v_loc = config_->run_online
                               ? output.chain->pose(*qdata.sid_loc)  // online
                               : T_v_m_loc.inverse();                // offline

    if (!config_->run_online) {
      // publish the aligned points
      PointCloudMsg scan_msg;
      pcl::toROSMsg(aligned_points, scan_msg);
      scan_msg.header.frame_id = "world";
      // scan_msg.header.stamp = rclcpp::Time(*qdata.stamp);
      scan_pub_->publish(scan_msg);

      // publish the submap for localization
      PointCloudMsg submap_msg;
      pcl::toROSMsg(map_point_cloud, submap_msg);
      submap_msg.header.frame_id = "world (offset)";
      // submap_msg.header.stamp = rclcpp::Time(*qdata.stamp);
      map_pub_->publish(submap_msg);
    }
#if false
    // publish the occupancy grid origin
    Eigen::Affine3d T((T_w_v_loc * T_r_v_loc.inverse()).matrix());
    auto tf_msg = tf2::eigenToTransform(T);
    tf_msg.header.frame_id = "world (offset)";
    // tf_msg.header.stamp = rclcpp::Time(*qdata.stamp);
    tf_msg.child_frame_id = "change detection";
    tf_bc_->sendTransform(tf_msg);

    // publish the occupancy grid
    auto costmap_msg = costmap->toCostMapMsg();
    costmap_msg.header.frame_id = "change detection";
    // costmap_msg.header.stamp = rclcpp::Time(*qdata.stamp);
    costmap_pub_->publish(costmap_msg);

    // publish the point cloud
    auto pointcloud_msg = costmap->toPointCloudMsg();
    pointcloud_msg.header.frame_id = "change detection";
    // pointcloud_msg.header.stamp = rclcpp::Time(*qdata.stamp);
    pointcloud_pub_->publish(pointcloud_msg);
#endif
  }

  /// output
#if false
  auto change_detection_costmap_ref = output.change_detection_costmap.locked();
  auto &change_detection_costmap = change_detection_costmap_ref.get();
  change_detection_costmap = costmap;
#endif

  CLOG(INFO, "lidar.change_detection")
      << "Change detection for lidar scan at stamp: " << stamp << " - DONE";
}

}  // namespace lidar
}  // namespace vtr