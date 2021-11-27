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
 * \file preprocessing_module_v2.hpp
 * \brief PreprocessingModuleV2 class definition
 *
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include "vtr_lidar/cache.hpp"
#include "vtr_tactic/modules/base_module.hpp"
#include "vtr_tactic/task_queue.hpp"

// visualization
#include <sensor_msgs/msg/point_cloud2.hpp>

namespace vtr {
namespace lidar {

/** \brief Preprocess raw pointcloud points and compute normals */
class PreprocessingModuleV2 : public tactic::BaseModule {
 public:
  using PointCloudMsg = sensor_msgs::msg::PointCloud2;

  /** \brief Static module identifier. */
  static constexpr auto static_name = "lidar.preprocessing_v2";

  /** \brief Config parameters. */
  struct Config {
    int num_threads = 1;
    float vertical_angle_res = 0.00745;
    float polar_r_scale = 1.5;
    float r_scale = 4.0;
    float h_scale = 0.5;
    float frame_voxel_size = 0.1;
    int num_sample1 = 100000;
    float min_norm_score1 = 0.0;
    int num_sample2 = 100000;
    float min_norm_score2 = 0.01;
    float min_normal_estimate_dist = 2.0;
    float max_normal_estimate_angle = 0.417;  // 5/12 original parameter value
    int cluster_num_sample = 100000;
    bool visualize = false;
  };

  PreprocessingModuleV2(const std::string &name = static_name)
      : tactic::BaseModule{name}, config_(std::make_shared<Config>()) {}

  void configFromROS(const rclcpp::Node::SharedPtr &node,
                     const std::string param_prefix) override;

 private:
  void runImpl(tactic::QueryCache &qdata, const tactic::Graph::Ptr &graph,
               const tactic::TaskExecutor::Ptr &executor) override;

  std::shared_ptr<Config> config_;

  /** \brief for visualization only */
  bool publisher_initialized_ = false;
  rclcpp::Publisher<PointCloudMsg>::SharedPtr filtered_pub_;
};

}  // namespace lidar
}  // namespace vtr