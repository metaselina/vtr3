#include <vtr_safety_monitor/inputs/heartbeat_monitor.hpp>
#include <vtr_logging/logging.hpp>

namespace vtr {
namespace safety_monitor {

HeartbeatMonitorInput::HeartbeatMonitorInput(const std::shared_ptr<rclcpp::Node> node)
    : SafetyMonitorInput(node) {
  signal_monitors.clear();
  double msg_timeout = 0; // Don't check for timeout of msg

  // Initialize heatbeat monitor.
  signal_monitors.emplace_back(SignalMonitor(node));
  signal_monitors.back().initializeType(DISCRETE_MONITOR);
  signal_monitors.back().initialize("Heartbeat", msg_timeout);

  // Initialize message subscriptions
  status_subscriber_ = node_->create_subscription<RobotStatus>("robot",
                                                               10,
                                                               std::bind(&HeartbeatMonitorInput::statusCallback,
                                                                         this,
                                                                         std::placeholders::_1));
  period_ = node_->declare_parameter<double>("heartbeat_period", 2.0);
  timer_ = rclcpp::create_timer(node_,
                                node_->get_clock(),
                                std::chrono::milliseconds((long) (1000.0
                                    * period_)),
                                std::bind(&HeartbeatMonitorInput::timedCallback,
                                          this));

  LOG(INFO) << " Heartbeat period: " << period_;

}

void HeartbeatMonitorInput::statusCallback(const RobotStatus::SharedPtr status) {
  if (status->state == "::Repeat::Follow") {
    signal_monitors[0].setMonitorDesiredAction(CONTINUE);
  }
  timer_->reset();
}

void HeartbeatMonitorInput::timedCallback() {
  signal_monitors[1].setMonitorDesiredAction(PAUSE);
  LOG(WARNING)
      << "Heartbeat monitor has not received a robot status for at least "
      << period_ << " seconds. Setting action to PAUSE";
}

} // safety_monitor
} // vtr
