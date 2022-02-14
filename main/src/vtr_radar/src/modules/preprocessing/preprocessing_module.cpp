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
 * \file preprocessing_module.cpp
 * \author Yuchen Wu, Keenan Burnett, Autonomous Space Robotics Lab (ASRL)
 */
#include "vtr_radar/modules/preprocessing/preprocessing_module.hpp"

#include "pcl_conversions/pcl_conversions.h"

#include "vtr_radar/features/normal.hpp"
#include "vtr_radar/filters/grid_subsampling.hpp"
#include "vtr_radar/utils.hpp"

namespace vtr {
namespace radar {

using namespace tactic;

auto PreprocessingModule::Config::fromROS(const rclcpp::Node::SharedPtr &node,
                                          const std::string &param_prefix)
    -> ConstPtr {
  auto config = std::make_shared<Config>();
  // clang-format off
  config->frame_voxel_size = node->declare_parameter<float>(param_prefix + ".frame_voxel_size", config->frame_voxel_size);

  config->window_size = node->declare_parameter<float>(param_prefix + ".window_size", config->window_size);
  config->azimuth_res = node->declare_parameter<float>(param_prefix + ".azimuth_res", config->azimuth_res);
  config->rho_scale = node->declare_parameter<float>(param_prefix + ".rho_scale", config->rho_scale);
  config->num_threads = node->declare_parameter<int>(param_prefix + ".num_threads", config->num_threads);

  config->num_sample_linearity = node->declare_parameter<int>(param_prefix + ".num_sample_linearity", config->num_sample_linearity);
  config->min_linearity_score = node->declare_parameter<float>(param_prefix + ".min_linearity_score", config->min_linearity_score);

  config->visualize = node->declare_parameter<bool>(param_prefix + ".visualize", config->visualize);
  // clang-format on
  return config;
}

void PreprocessingModule::run_(QueryCache &qdata0, OutputCache &,
                               const Graph::Ptr &, const TaskExecutor::Ptr &) {
  auto &qdata = dynamic_cast<RadarQueryCache &>(qdata0);

  /// Create a node for visualization if necessary
  if (config_->visualize && !publisher_initialized_) {
    // clang-format off
    filtered_pub_ = qdata.node->create_publisher<PointCloudMsg>("filtered_point_cloud", 5);
    marker_pub_ = qdata.node->create_publisher<MarkerMsg>("marker_deleter", 5);
    normal_pub_ = qdata.node->create_publisher<MarkerArrayMsg>("filtered_normal", 5);
    // clang-format on
    publisher_initialized_ = true;
  }

  // Get input point cloud
  const auto point_cloud = qdata.raw_point_cloud.ptr();

  if (point_cloud->size() == 0) {
    std::string err{"Empty point cloud."};
    CLOG(ERROR, "radar.preprocessing") << err;
    throw std::runtime_error{err};
  }

  CLOG(DEBUG, "radar.preprocessing")
      << "raw point cloud size: " << point_cloud->size();

  auto filtered_point_cloud =
      std::make_shared<pcl::PointCloud<PointWithInfo>>(*point_cloud);

  /// Grid subsampling

  // Get subsampling of the frame in carthesian coordinates
  gridSubsamplingCentersV2(*filtered_point_cloud, config_->frame_voxel_size);

  CLOG(DEBUG, "radar.preprocessing")
      << "grid subsampled point cloud size: " << filtered_point_cloud->size();

  /// Compute normals

  // Define the polar neighbors radius in the scaled polar coordinates
  float radius = config_->window_size * config_->azimuth_res;

  // Extracts normal vectors of sampled points
  auto norm_scores = extractNormal(point_cloud, filtered_point_cloud, radius,
                                   config_->rho_scale, config_->num_threads);

  /// Filtering based on normal scores (linearity)

  // Remove points with a low normal score
  auto sorted_norm_scores = norm_scores;
  std::sort(sorted_norm_scores.begin(), sorted_norm_scores.end());
  float min_score = sorted_norm_scores[std::max(
      0, (int)sorted_norm_scores.size() - config_->num_sample_linearity)];
  min_score = std::max(config_->min_linearity_score, min_score);
  if (min_score >= 0) {
    std::vector<int> indices;
    indices.reserve(filtered_point_cloud->size());
    int i = 0;
    for (const auto &point : *filtered_point_cloud) {
      if (point.normal_score >= min_score) indices.emplace_back(i);
      i++;
    }
    *filtered_point_cloud =
        pcl::PointCloud<PointWithInfo>(*filtered_point_cloud, indices);
  }

  CLOG(DEBUG, "lidar.preprocessing")
      << "linearity sampled point size: " << filtered_point_cloud->size();

  CLOG(DEBUG, "radar.preprocessing")
      << "final subsampled point size: " << filtered_point_cloud->size();

  if (config_->visualize) {
    auto point_cloud_tmp = *filtered_point_cloud;
    const auto ref_time = (double)(*qdata.stamp / 1000) / 1e6;
    std::for_each(point_cloud_tmp.begin(), point_cloud_tmp.end(),
                  [&](PointWithInfo &point) { point.time -= ref_time; });
    auto pc2_msg = std::make_shared<PointCloudMsg>();
    pcl::toROSMsg(point_cloud_tmp, *pc2_msg);
    pc2_msg->header.frame_id = "radar";
    pc2_msg->header.stamp = rclcpp::Time(*qdata.stamp);
    filtered_pub_->publish(*pc2_msg);

    MarkerArrayMsg ma_msg;
    for (int id = 0; id < 1000; ++id) {
      auto &m = ma_msg.markers.emplace_back();
      m.header.frame_id = "radar";
      m.header.stamp = rclcpp::Time(*qdata.stamp);
      m.ns = "normal";
      m.id = id;
      m.type = MarkerMsg::ARROW;
      if (filtered_point_cloud->size() > id) {
        const auto &point = filtered_point_cloud->at(id);
        m.action = MarkerMsg::ADD;
        auto &sp = m.points.emplace_back();
        sp.x = point.x;
        sp.y = point.y;
        sp.z = point.z;
        auto &ep = m.points.emplace_back();
        ep.x = point.x + point.normal_x * 5.0;
        ep.y = point.y + point.normal_y * 5.0;
        ep.z = point.z + point.normal_z * 5.0;
        m.scale.x = 0.5;
        m.scale.y = 1.5;
        m.scale.z = 1.0;
        m.color.r = 1.0f;
        m.color.g = 0.0f;
        m.color.b = 0.0f;
        m.color.a = 1.0;
      } else {
        m.action = MarkerMsg::DELETE;
      }
    }
    normal_pub_->publish(ma_msg);
  }

  /// Output
  qdata.preprocessed_point_cloud = filtered_point_cloud;
}

}  // namespace radar
}  // namespace vtr