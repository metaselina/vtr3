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
 * \file test_utils.hpp
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#pragma once

#include "vtr_mission_planning_v2/mission_server/mission_server.hpp"
#include "vtr_mission_planning_v2/state_machine/state_machine.hpp"

using namespace std::chrono_literals;
using namespace vtr;
using namespace vtr::mission_planning;

namespace vtr {
namespace mission_planning {

struct TestTactic : public StateMachine::Tactic {
  PTR_TYPEDEFS(TestTactic);

  UniqueLock lockPipeline() override {
    LOG(WARNING) << "Locking pipeline";
    return UniqueLock();
  }

  void setPipeline(const tactic::PipelineMode& pipeline) override {
    LOG(WARNING) << "Switching pipeline to " << pipeline;
  }

  void setPath(const tactic::PathType& path, bool follow = false) override {
    LOG(WARNING) << "Setting path to " << path << " with follow " << follow;
  }

  /// Called when starting a new teach/repeat
  void addRun(bool) override { LOG(WARNING) << "Adding a new run"; }
  /// Called when finishing a teach (should be called whenever finishing)
  void relaxGraph() override { LOG(WARNING) << "Optimize the graph a new run"; }
  /// Called when finishing a teach/repeat
  void saveGraph() override {}
  /// Called when trying to merge into existing path
  bool canCloseLoop() const override {
    LOG(WARNING) << "Asking if can close loop, return yes";
    return true;
  }
  void connectToTrunk(bool, bool) override {
    LOG(WARNING) << "Connecting to trunk";
  }

  void setTrunk(const tactic::VertexId&) override {}
  double distanceToSeqId(const uint64_t&) override { return 9001; }
  bool pathFollowingDone() override { return true; }
  const tactic::Localization& persistentLoc() const override { return loc_; }

  tactic::PipelineMode pipeline_;
  tactic::Localization loc_;
};

struct TestRoutePlanner : public RoutePlannerInterface {
  PTR_TYPEDEFS(TestRoutePlanner);
  // clang-format off
  PathType path(const VertexId&, const VertexId&) override { return PathType{}; }
  PathType path(const VertexId&, const VertexId::List&, std::list<uint64_t>*) override {return PathType();}
  // clang-format on
};

struct TestCallback : public StateMachineCallback {
  PTR_TYPEDEFS(TestCallback);
  void stateSuccess() override {
    LOG(WARNING) << "State success has been notified!";
  }
  void stateUpdate(const double) override {
    LOG(WARNING) << "State update has been notified!";
  }
};

struct TestStateMachine : public StateMachineInterface {
 public:
  PTR_TYPEDEFS(TestStateMachine);

  TestStateMachine(const StateMachineCallback::Ptr& callback)
      : StateMachineInterface(callback) {}

  void handle(const Event::Ptr& event = std::make_shared<Event>(),
              const bool block = false) override {
    CLOG(INFO, "mission.state_machine")
        << "Handling event: " << event << ", block: " << block;
  }

  StateMachineCallback::Ptr callback() const {
    return StateMachineInterface::callback();
  }
};

struct TestGoalHandle {
  TestGoalHandle(const int& id0 = 0,
                 const GoalTarget& target0 = GoalTarget::Unknown,
                 const std::chrono::milliseconds& pause_before0 = 0ms,
                 const std::chrono::milliseconds& pause_after0 = 0ms,
                 const std::list<tactic::VertexId>& path0 = {})
      : id(id0),
        target(target0),
        pause_before(pause_before0),
        pause_after(pause_after0),
        path(path0) {}

  int id;
  GoalTarget target;
  std::chrono::milliseconds pause_before;
  std::chrono::milliseconds pause_after;
  std::list<tactic::VertexId> path;
};

}  // namespace mission_planning
}  // namespace vtr