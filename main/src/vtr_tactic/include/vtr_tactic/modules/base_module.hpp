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
 * \file base_module.hpp
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include <boost/uuid/uuid.hpp>

#include "rclcpp/rclcpp.hpp"

#include "vtr_common/timing/stopwatch.hpp"
#include "vtr_logging/logging.hpp"
#include "vtr_tactic/cache.hpp"
#include "vtr_tactic/types.hpp"

namespace vtr {
namespace tactic {

class ModuleFactory;
class TaskExecutor;

class BaseModule : public std::enable_shared_from_this<BaseModule> {
 public:
  PTR_TYPEDEFS(BaseModule);

  /** \brief An unique identifier. Subclass should overwrite this. */
  static constexpr auto static_name = "base_module";

  struct Config {
    PTR_TYPEDEFS(Config);

    virtual ~Config() = default;  // for polymorphism

    /// sub-module must implement this function
    static Ptr fromROS(const rclcpp::Node::SharedPtr &, const std::string &);
  };

  BaseModule(const std::shared_ptr<ModuleFactory> &module_factory = nullptr,
             const std::string &name = static_name);

  virtual ~BaseModule();

  /**
   * \brief Gets the identifier of the module instance at runtime.
   * \details The identifier is the string passed to the BaseModule constructor.
   */
  const std::string &name() const { return name_; }

  /** \brief Runs the module with timing. */
  void run(QueryCache &qdata, OutputCache &output, const Graph::Ptr &graph,
           const std::shared_ptr<TaskExecutor> &executor);

  /** \brief Runs the module asynchronously with timing. */
  void runAsync(QueryCache &qdata, OutputCache &output, const Graph::Ptr &graph,
                const std::shared_ptr<TaskExecutor> &executor,
                const size_t &priority, const boost::uuids::uuid &dep_id);

  /** \brief Resets the module's internal state. */
  virtual void reset() {}

 protected:
  std::shared_ptr<ModuleFactory> factory() const;

 private:
  /** \brief Runs the module. */
  virtual void run_(QueryCache &, OutputCache &, const Graph::Ptr &,
                    const std::shared_ptr<TaskExecutor> &) = 0;

  /** \brief Runs the module asynchronously. */
  virtual void runAsync_(QueryCache &, OutputCache &, const Graph::Ptr &,
                         const std::shared_ptr<TaskExecutor> &, const size_t &,
                         const boost::uuids::uuid &) {}

 private:
  const std::weak_ptr<ModuleFactory> module_factory_;

  /** \brief Name of the module assigned at runtime. */
  const std::string name_;

  /** \brief counter&timer that measures total runtime and average run time */
  common::timing::Stopwatch<> timer_{false};
  common::timing::Stopwatch<boost::chrono::thread_clock> thread_timer_{false};
  std::atomic<int> count_{0};

  /// factory handlers (note: local static variable constructed on first use)
 private:
  /** \brief a map from type_str trait to a constructor function */
  using CtorFunc = std::function<Ptr(const Config::ConstPtr &,
                                     const std::shared_ptr<ModuleFactory> &)>;
  using Name2Ctor = std::unordered_map<std::string, CtorFunc>;
  static Name2Ctor &name2Ctor() {
    static Name2Ctor name2ctor;
    return name2ctor;
  }

  /** \brief a map from type_str trait to a config from ROS function */
  using CfROSFunc = std::function<Config::ConstPtr(
      const rclcpp::Node::SharedPtr &, const std::string &)>;
  using Name2CfROS = std::unordered_map<std::string, CfROSFunc>;
  static Name2CfROS &name2Cfros() {
    static Name2CfROS name2cfros;
    return name2cfros;
  }

  template <typename T>
  friend class ModuleRegister;
  friend class ModuleFactory;
  friend class ROSModuleFactory;
};

template <typename T>
struct ModuleRegister {
  ModuleRegister() {
    bool success = true;
    success &=
        BaseModule::name2Ctor()
            .try_emplace(
                T::static_name,
                BaseModule::CtorFunc(
                    [](const BaseModule::Config::ConstPtr &config,
                       const std::shared_ptr<ModuleFactory> &factory) {
                      const auto &config_typed =
                          (config == nullptr
                               ? std::make_shared<const typename T::Config>()
                               : std::dynamic_pointer_cast<
                                     const typename T::Config>(config));
                      return std::make_shared<T>(config_typed, factory);
                    }))
            .second;
    success &=
        BaseModule::name2Cfros()
            .try_emplace(
                T::static_name,
                BaseModule::CfROSFunc([](const rclcpp::Node::SharedPtr &node,
                                         const std::string &prefix) {
                  return T::Config::fromROS(node, prefix);
                }))
            .second;
    if (!success)
      throw std::runtime_error{"ModuleRegister failed - duplicated name"};
  }
};

/// \brief Register a module
#define VTR_REGISTER_MODULE_DEC_TYPE(NAME) \
  inline static vtr::tactic::ModuleRegister<NAME> reg_

}  // namespace tactic
}  // namespace vtr
