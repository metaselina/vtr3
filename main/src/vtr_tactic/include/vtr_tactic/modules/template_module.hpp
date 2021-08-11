#pragma once

#include <vtr_tactic/modules/base_module.hpp>

namespace vtr {
namespace tactic {

/** \brief A tactic module template */
class TemplateModule : public BaseModule {
 public:
  /** \brief Static module identifier. */
  static constexpr auto static_name = "template";

  /** \brief Collection of config parameters */
  struct Config {
    std::string parameter = "default value";
  };

  TemplateModule(const std::string &name = static_name)
      : BaseModule{name}, config_(std::make_shared<Config>()) {}

  void configFromROS(const rclcpp::Node::SharedPtr &node,
                     const std::string param_prefix) override {
    /// Configure your module from ROS
    config_ = std::make_shared<Config>();
    // clang-format off
    config_->parameter = node->declare_parameter<std::string>(param_prefix + ".parameter", config_->parameter);
    // clang-format on
    CLOG(INFO, "tactic.module")
        << "Template module parameter set to: " << config_->parameter;
  }

 private:
  void runImpl(QueryCache &, const Graph::ConstPtr &) override {
    /// Pure virtual method that must be overriden.
    /// Do the actual work of your module. Load data from and store data to
    /// QueryCache.
    CLOG(INFO, "tactic.module") << "Running the template module...";
  }

  void updateGraphImpl(QueryCache &, const Graph::Ptr &, VertexId) override {
    /// Override this method if your module needs to store data into the graph.
    CLOG(INFO, "tactic.module")
        << "Template module is updating the pose graph...";
  }

  void visualizeImpl(QueryCache &, const Graph::ConstPtr &,
                     std::mutex &) override {
    /// Override this method if you module produces visualization. The mutex is
    /// for OpenCV.
    CLOG(INFO, "tactic.module") << "Template module is being visualized...";
  }

  /** \brief Module configuration. */
  std::shared_ptr<Config> config_;  /// \todo no need to be a shared pointer.
};

}  // namespace tactic
}  // namespace vtr