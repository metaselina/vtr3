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
 * \file teb_path_planner.hpp
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include <cstdio>
#include <iostream>
#include <cmath>
#include <vector>
#include <tuple>
#include <algorithm>
#include <random>
#include <chrono> // For benchmarking
#include <memory>


#include "rclcpp/rclcpp.hpp"

#include "nav_msgs/msg/path.hpp"
#include "tf2_ros/transform_broadcaster.h"
#include "std_msgs/msg/string.hpp"

#include "vtr_path_planning/cbit/cbit_config.hpp"
#include "vtr_path_planning/base_path_planner.hpp"
#include "vtr_path_planning/cbit/utils.hpp"
#include "vtr_path_planning/cbit/generate_pq.hpp"
#include "vtr_path_planning/cbit/cbit_path_planner.hpp"
#include "vtr_path_planning/cbit/cbit_costmap.hpp"


namespace vtr {
namespace path_planning {

class CBIT : public BasePathPlanner {
 public:
  PTR_TYPEDEFS(CBIT);

  static constexpr auto static_name = "cbit";

  // Note all rosparams that are in the config yaml file need to be declared here first, though they can be then changes using the declareparam function for ros in the cpp file
  struct Config : public BasePathPlanner::Config {
    PTR_TYPEDEFS(Config);
  
    // point, circular, line, two_circles, polygon
    std::string robot_model = "point";
    double robot_radius = 0.5;

    // CBIT
    // Environment
    double obs_padding = 1.0;
    int curv_to_euclid_discretization = 20;
    double sliding_window_width = 15.0;
    double sliding_window_freespace_padding = 0.5;
    double corridor_resolution = 0.05;
    double state_update_freq = 1; // In Hz
    bool update_state = true;
    int rand_seed = 1;

    // ROC
    double roc_lookahead = 5.0;
    int roc_discretization = 40;
    double roc_q_tolerance = 0.001;

    // Planner Tuning Params
    int initial_samples = 250;
    int batch_samples = 100;
    double pre_seed_resolution = 0.5;
    double alpha = 0.5;
    double q_max = 2.5;
    int frame_interval = 50;
    int iter_max = 10000000;
    double eta = 1.1;
    double rad_m_exhange = 1.00;
    double initial_exp_rad = 1.00;
    bool extrapolation = true;

    // Misc
    bool incremental_plotting = false;
    bool plotting = true; 

    static Ptr fromROS(const rclcpp::Node::SharedPtr& node,
                       const std::string& prefix = "path_planning");
    // Subscription for parameter change
    rclcpp::AsyncParametersClient::SharedPtr ros_parameters_client;
    rclcpp::Subscription<rcl_interfaces::msg::ParameterEvent>::SharedPtr
        ros_parameter_event_sub;
  };

  CBIT(const Config::ConstPtr& config,
                 const RobotState::Ptr& robot_state,
                 const Callback::Ptr& callback);
  ~CBIT() override;

 protected:
  void initializeRoute(RobotState& robot_state) override; // Declare this as virtual so that the Lidarcbit can override it if using that static name
  //void initializeRouteTest(RobotState& robot_state) override;
  Command computeCommand(RobotState& robot_state) override;
  void visualize(std::string text, const tactic::Timestamp& stamp, const tactic::EdgeTransform& T_w_p,const tactic::EdgeTransform& T_p_r);

 protected:
  struct ChainInfo {
    tactic::Timestamp stamp;
    Eigen::Matrix<double, 6, 1> w_p_r_in_r;
    // planning frame is the current localization frame
    tactic::EdgeTransform T_p_r;  // T_planning_robot
    tactic::EdgeTransform T_w_p;  // T_world_planning
    unsigned curr_sid;
  };
  /** \brief Retrieve information for planning from localization chain */
  ChainInfo getChainInfo(RobotState& robot_state);

 //private: // I think we need to make all of these protected so the derived lidarcbit class can access them
  const Config::ConstPtr config_;
  CBITConfig cbit_config;
  VTR_REGISTER_PATH_PLANNER_DEC_TYPE(CBIT);

  // Ros2 publishers
  std::shared_ptr<tf2_ros::TransformBroadcaster> tf_bc_;
  rclcpp::Publisher<nav_msgs::msg::Path>::SharedPtr path_pub_;
  rclcpp::Publisher<std_msgs::msg::String>::SharedPtr test_pub_;


  // TEST: Initializing a pointer to shared memory, want to see if the virtual functions can access it
  // TODO: Long term we probably are going to want to format the output as transforms on the planner side and then adjust the class types here accordingly
  std::vector<Pose> cbit_path;
  std::shared_ptr<std::vector<Pose>> cbit_path_ptr;


  // Create costmap pointer object
  std::shared_ptr<CBITCostmap> costmap_ptr = std::make_shared<CBITCostmap> ();


};

}  // namespace path_planning
}  // namespace vtr
