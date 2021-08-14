#pragma once

#include <geometry_msgs/msg/twist.hpp>
#include <rclcpp/rclcpp.hpp>

#include <vtr_safety_monitor/safety_monitor/base_monitor.hpp>
#include <vtr_safety_monitor/safety_monitor/deadman_monitor.hpp>
#include <vtr_safety_monitor/safety_monitor/heartbeat_monitor.hpp>

#include <vtr_messages/msg/robot_status.hpp>

namespace vtr {
namespace safety_monitor {

using TwistMsg = geometry_msgs::msg::Twist;

class SafetyMonitor {
 public:
  SafetyMonitor(const rclcpp::Node::SharedPtr &node);

 private:
  void getSafetyStatus();
  void processCommand(const TwistMsg::SharedPtr msg);

  /**
   * \brief Converts strings provided in the launch file into safety monitor
   * objects
   */
  std::unique_ptr<BaseMonitor> createMonitor(
      const std::string &monitor_input_str);

  // Parameters
  int monitor_update_period_;
  std::vector<std::string> monitor_names_;
  double absolute_max_speed_;

  // List of Monitors
  std::vector<std::unique_ptr<BaseMonitor>> monitors_;

  std::mutex status_mutex_;
  double speed_limit_ = 0.0;
  int desired_action_ = PAUSE;

  /** \brief ROS-handle for communication */
  const rclcpp::Node::SharedPtr node_;
  // Objects for periodic status updates
  rclcpp::TimerBase::SharedPtr safety_status_timer_;

  rclcpp::Subscription<TwistMsg>::SharedPtr command_sub_;
  rclcpp::Publisher<TwistMsg>::SharedPtr command_pub_;
};

}  // namespace safety_monitor
}  // namespace vtr
