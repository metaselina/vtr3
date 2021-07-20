#include <vtr_tactic/modules/lidar/preprocessing_module.hpp>

namespace {
PointCloudMsg::SharedPtr toROSMsg(const std::vector<PointXYZ> &points,
                                  const std::vector<float> &intensities,
                                  const std::string &frame_id,
                                  const rclcpp::Time &timestamp) {
  pcl::PointCloud<pcl::PointXYZI> cloud;
  auto pcitr = points.begin();
  auto ititr = intensities.begin();
  for (; pcitr != points.end(); pcitr++, ititr++) {
    pcl::PointXYZI pt;
    pt.x = pcitr->x;
    pt.y = pcitr->y;
    pt.z = pcitr->z;
    pt.intensity = *ititr;
    cloud.points.push_back(pt);
  }

  auto pc2_msg = std::make_shared<PointCloudMsg>();
  pcl::toROSMsg(cloud, *pc2_msg);
  pc2_msg->header.frame_id = frame_id;
  pc2_msg->header.stamp = timestamp;
  return pc2_msg;
}
}  // namespace

namespace vtr {
namespace tactic {
namespace lidar {

void PreprocessingModule::configFromROS(const rclcpp::Node::SharedPtr &node,
                                        const std::string param_prefix) {
  config_ = std::make_shared<Config>();
  // clang-format off
  config_->num_threads = node->declare_parameter<int>(param_prefix + ".num_threads", config_->num_threads);
#ifdef DETERMINISTIC_VTR
  LOG_IF(config_->num_threads != 1, WARNING) << "Point cloud pre-processor number of threads set to 1 in deterministic mode.";
  config_->num_threads = 1;
#endif
  config_->num_channels = node->declare_parameter<int>(param_prefix + ".num_channels", config_->num_channels);
  config_->vertical_angle_res = node->declare_parameter<float>(param_prefix + ".vertical_angle_res", config_->vertical_angle_res);
  config_->polar_r_scale = node->declare_parameter<float>(param_prefix + ".polar_r_scale", config_->polar_r_scale);
  config_->r_scale = node->declare_parameter<float>(param_prefix + ".r_scale", config_->r_scale);
  config_->h_scale = node->declare_parameter<float>(param_prefix + ".h_scale", config_->h_scale);
  config_->frame_voxel_size = node->declare_parameter<float>(param_prefix + ".frame_voxel_size", config_->frame_voxel_size);

  config_->num_sample1 = node->declare_parameter<int>(param_prefix + ".num_sample1", config_->num_sample1);
  config_->min_norm_score1 = node->declare_parameter<float>(param_prefix + ".min_norm_score1", config_->min_norm_score1);
  config_->num_sample2 = node->declare_parameter<int>(param_prefix + ".num_sample2", config_->num_sample2);
  config_->min_norm_score2 = node->declare_parameter<float>(param_prefix + ".min_norm_score2", config_->min_norm_score2);

  config_->visualize = node->declare_parameter<bool>(param_prefix + ".visualize", config_->visualize);
  // clang-format on
}

void PreprocessingModule::runImpl(QueryCache &qdata, MapCache &,
                                  const Graph::ConstPtr &) {
  /// Create a node for visualization if necessary
  if (config_->visualize && !pc_pub_)
    pc_pub_ = qdata.node->create_publisher<PointCloudMsg>("sampled_points", 20);

  // Get input point cloud
  auto &points = *qdata.raw_pointcloud;
  auto &points_time = *qdata.raw_pointcloud_time;

  LOG(INFO) << "[lidar.preprocessing] raw point cloud size: " << points.size();

  /// Grid subsampling

  // Get subsampling of the frame in carthesian coordinates
  std::vector<PointXYZ> sampled_points;
  std::vector<size_t> sampled_inds;
  vtr::lidar::grid_subsampling_centers(points, sampled_points, sampled_inds,
                                       config_->frame_voxel_size);

  // Filter time
  std::vector<double> sampled_points_time;
  sampled_points_time.reserve(sampled_points.size());
  for (auto &ind : sampled_inds) {
    sampled_points_time.push_back(points_time[ind]);
  }

  LOG(INFO) << "[lidar.preprocessing] grid subsampled point cloud size: "
            << sampled_points.size();

  /// Compute normals and an icp score

  // Create a copy of points in polar coordinates
  std::vector<PointXYZ> polar_points(points);
  vtr::lidar::cart2pol_(polar_points, true);

  // Convert sampled_points to polar and rescale
  std::vector<PointXYZ> sampled_polar_points0;
  sampled_polar_points0.reserve(sampled_points.size());
  for (auto &ind : sampled_inds) {
    sampled_polar_points0.push_back(polar_points[ind]);
  }
  std::vector<PointXYZ> sampled_polar_points(sampled_polar_points0);

  // Apply scale to radius and angle horizontal
  vtr::lidar::lidar_log_radius(polar_points, config_->r_scale);
  vtr::lidar::lidar_horizontal_scale(polar_points, config_->h_scale);
  vtr::lidar::lidar_log_radius(sampled_polar_points, config_->r_scale);
  vtr::lidar::lidar_horizontal_scale(sampled_polar_points, config_->h_scale);

  // Define the polar neighbors radius in the scaled polar coordinates
  float polar_r = config_->polar_r_scale * config_->vertical_angle_res;

  // Extract normal vectors of sampled points
  std::vector<PointXYZ> normals;
  std::vector<float> norm_scores;
  vtr::lidar::extract_lidar_frame_normals(
      points, polar_points, sampled_points, sampled_polar_points, normals,
      norm_scores, polar_r, config_->num_threads);

  // Better normal score based on distance and incidence angle
  std::vector<float> icp_scores(norm_scores);
  vtr::lidar::smart_icp_score(sampled_polar_points0, icp_scores);

  /// Filtering based on normal scores (planarity)

  // Remove points with a low normal score
  auto sorted_norm_scores = norm_scores;
  std::sort(sorted_norm_scores.begin(), sorted_norm_scores.end());
  float min_score = sorted_norm_scores[std::max(
      0, (int)sorted_norm_scores.size() - config_->num_sample1)];
  min_score = std::max(config_->min_norm_score1, min_score);
  if (min_score >= 0) {
    filter_pointcloud(sampled_points, norm_scores, min_score);
    filter_pointcloud(normals, norm_scores, min_score);
    filter_floatvector(sampled_points_time, norm_scores, min_score);
    filter_floatvector(icp_scores, norm_scores, min_score);
    filter_floatvector(norm_scores, min_score);
  }

  LOG(INFO) << "[lidar.preprocessing] planarity sampled point size: "
            << sampled_points.size();

  /// Filtering based on a normal directions

  vtr::lidar::smart_normal_score(sampled_points, sampled_polar_points0, normals,
                                 norm_scores);

  sorted_norm_scores = norm_scores;
  std::sort(sorted_norm_scores.begin(), sorted_norm_scores.end());
  min_score = sorted_norm_scores[std::max(
      0, (int)sorted_norm_scores.size() - config_->num_sample2)];
  min_score = std::max(config_->min_norm_score2, min_score);
  if (min_score >= 0) {
    filter_pointcloud(sampled_points, norm_scores, min_score);
    filter_pointcloud(normals, norm_scores, min_score);
    filter_floatvector(sampled_points_time, norm_scores, min_score);
    filter_floatvector(icp_scores, norm_scores, min_score);
    filter_floatvector(norm_scores, min_score);
  }

  LOG(INFO) << "[lidar.preprocessing] final subsampled point size: "
            << sampled_points.size();
  if (config_->visualize)
    pc_pub_->publish(
        *toROSMsg(sampled_points, norm_scores, "velodyne", *qdata.rcl_stamp));

  /// Output
  qdata.preprocessed_pointcloud.fallback(sampled_points);
  qdata.preprocessed_pointcloud_time.fallback(sampled_points_time);
  qdata.normals.fallback(normals);
  qdata.normal_scores.fallback(norm_scores);
  qdata.icp_scores.fallback(icp_scores);
}

void PreprocessingModule::visualizeImpl(QueryCache &qdata, MapCache &,
                                        const Graph::ConstPtr &, std::mutex &) {
}

}  // namespace lidar
}  // namespace tactic
}  // namespace vtr