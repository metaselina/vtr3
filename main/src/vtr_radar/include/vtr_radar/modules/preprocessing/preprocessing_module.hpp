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
 * \file preprocessing_module.hpp
 * \author Yuchen Wu, Keenan Burnett, Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include "sensor_msgs/msg/point_cloud2.hpp"
#include "visualization_msgs/msg/marker_array.hpp"

#include "vtr_radar/cache.hpp"
#include "vtr_tactic/modules/base_module.hpp"
#include "vtr_tactic/task_queue.hpp"

namespace vtr {
namespace radar {

/** \brief Preprocess raw pointcloud points and compute normals */
class PreprocessingModule : public tactic::BaseModule {
 public:
  using PointCloudMsg = sensor_msgs::msg::PointCloud2;
  using MarkerMsg = visualization_msgs::msg::Marker;
  using MarkerArrayMsg = visualization_msgs::msg::MarkerArray;

  /** \brief Static module identifier. */
  static constexpr auto static_name = "radar.preprocessing";

  /** \brief Config parameters. */
  struct Config : public tactic::BaseModule::Config {
    PTR_TYPEDEFS(Config);

    float frame_voxel_size = 0.1;

    float window_size = 3.0;
    float azimuth_res = 0.016;  // radar azimuth resolution in radians
    float rho_scale = 1.0;      // scale factor for range measurement rho
    int num_threads = 1;

    int num_sample_linearity = 500;
    float min_linearity_score = 0.5;

    bool visualize = false;

    static ConstPtr fromROS(const rclcpp::Node::SharedPtr &node,
                            const std::string &param_prefix);
  };

  PreprocessingModule(
      const Config::ConstPtr &config,
      const std::shared_ptr<tactic::ModuleFactory> &module_factory = nullptr,
      const std::string &name = static_name)
      : tactic::BaseModule{module_factory, name}, config_(config) {}

 private:
  void run_(tactic::QueryCache &qdata, tactic::OutputCache &output,
            const tactic::Graph::Ptr &graph,
            const tactic::TaskExecutor::Ptr &executor) override;

  Config::ConstPtr config_;

  /** \brief for visualization only */
  bool publisher_initialized_ = false;
  rclcpp::Publisher<PointCloudMsg>::SharedPtr filtered_pub_;
  rclcpp::Publisher<MarkerMsg>::SharedPtr marker_pub_;
  rclcpp::Publisher<MarkerArrayMsg>::SharedPtr normal_pub_;

  VTR_REGISTER_MODULE_DEC_TYPE(PreprocessingModule);
};

}  // namespace radar
}  // namespace vtr