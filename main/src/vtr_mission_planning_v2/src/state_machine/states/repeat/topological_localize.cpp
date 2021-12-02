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
 * \file topological_localize.cpp
 * \brief
 *
 * \author Yuchen Wu, Autonomous Space Robotics Lab (ASRL)
 */
#include "vtr_mission_planning_v2/state_machine/states/repeat/topological_localize.hpp"

namespace vtr {
namespace mission_planning {
namespace repeat {

StateInterface::Ptr TopologicalLocalize::nextStep(
    const StateInterface &new_state) const {
  // If where we are going is not a child, delegate to the parent
  if (!InChain(new_state)) return Parent::nextStep(new_state);

  // If we aren't changing to a different chain, there is no intermediate step
  return nullptr;
}

StateInterface::Ptr TopologicalLocalize::entryState() const { return nullptr; }

void TopologicalLocalize::processGoals(StateMachine &state_machine,
                                       const Event &event) {
  switch (event.signal) {
    case Signal::Continue:
      break;
    default:
      return Parent::processGoals(state_machine, event);
  }

  switch (event.action) {
    case Action::Continue:
      /// \todo not thread safe
      if (getTactic(state_machine)->persistentLoc().v.isSet()) {
        return Parent::processGoals(state_machine, Event(Action::EndGoal));
      } else {
        std::string err{
            "Attempted to repeat without a persistent localization set!"};
        CLOG(ERROR, "mission.state_machine") << err;
        throw std::runtime_error(err);
        return Parent::processGoals(state_machine, Event(Action::Abort));
      }
      // NOTE: the lack of a break statement here is intentional, to allow
      // unhandled cases to percolate up the chain
    default:
      // Delegate all goal swapping/error handling to the base class
      return Parent::processGoals(state_machine, event);
  }
}

void TopologicalLocalize::onExit(StateMachine &state_machine,
                                 StateInterface &new_state) {
  // If the new target is a derived class, we are not exiting
  if (InChain(new_state)) return;

  // Note: This is called *before* we call up the tree, as we destruct from
  // leaves to root

  // Recursively call up the inheritance chain until we get to the least common
  // ancestor
  Parent::onExit(state_machine, new_state);
}

void TopologicalLocalize::onEntry(StateMachine &state_machine,
                                  StateInterface &old_state) {
  // If the previous state was a derived class, we did not leave
  if (InChain(old_state)) return;

  // Recursively call up the inheritance chain until we get to the least common
  // ancestor
  Parent::onEntry(state_machine, old_state);
}

}  // namespace repeat
}  // namespace mission_planning
}  // namespace vtr
