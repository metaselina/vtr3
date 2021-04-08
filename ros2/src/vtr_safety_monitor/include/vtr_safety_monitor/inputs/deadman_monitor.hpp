#pragma once

#include <rclcpp/rclcpp.hpp>
#include <std_msgs/msg/string.hpp>
#include <sensor_msgs/msg/joy.hpp>

#include <vtr_safety_monitor/base/safety_monitor_input_base.hpp>


enum FOLLOWING_MODE {
  AUTO,
  MANUAL
};

namespace vtr {
namespace safety_monitor {

class DeadmanMonitorInput : public SafetyMonitorInput {
 public :
  DeadmanMonitorInput(ros::NodeHandle nh);
  void gamepadCallback(const sensor_msgs::msg::Joy::Ptr &msg);
  const static int axis_array[16];
  const static int value_array[16];

 private :
  ros::Subscriber gamepadSubscriber_;

  int deadman_button_index_;
  int pause_button_index_;
  bool deadman_pressed_;
  FOLLOWING_MODE followingMode_;

  int deadman_monitor;

  // the state of the cheat code.
  int codeState_;
  // the timestamp when hte last correct value in the code sequence was added.
  ros::Time timeofLastCodeSequence_;

  // The state when the cheat code is active.
  static const int CHEAT_CODE_ACTIVATED = 7777;

  void checkCodeState(const sensor_msgs::msg::Joy::Ptr &msg);

};

} // safety_monitor
} // vtr
