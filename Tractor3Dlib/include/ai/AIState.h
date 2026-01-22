/**
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <memory>
#include <string>

#include "core/Factory.h"

namespace tractor
{

class AIAgent;
class AIStateMachine;
class AIState;

/** Shared pointer type for AIState. */
using AIStatePtr = std::shared_ptr<AIState>;

/** Weak pointer type for AIState. */
using AIStateWeakPtr = std::weak_ptr<AIState>;

/**
 * Defines a single state in an AIStateMachine.
 *
 * An AIState encapsulates a state and unit of work within an AI
 * state machine. Events can be programmed or scripted when the
 * state is entered, exited and each frame/tick in its update event.
 */
class AIState : public StandaloneCreatableWithKey<AIState>
{
    friend class AIStateMachine;

  public:
    /**
     * Interface for listening to AIState events.
     */
    class Listener
    {
      public:
        /**
         * Virtual destructor.
         */
        virtual ~Listener() = default;

        /**
         * Called when a state is entered.
         *
         * @param agent The AIAgent this state event is for.
         * @param state The state that was entered.
         */
        virtual void stateEnter(AIAgent* agent, AIState* state);

        /**
         * Called when a state is exited.
         *
         * @param agent The AIAgent this state event is for.
         * @param state The state that was exited.
         */
        virtual void stateExit(AIAgent* agent, AIState* state);

        /**
         * Called once per frame when for a state when it is active.
         *
         * This method is normally where the logic for a state is implemented.
         *
         * @param agent The AIAgent this state event is for.
         * @param state The active AIState.
         * @param elapsedTime The elapsed time, in milliseconds.
         */
        virtual void stateUpdate(AIAgent* agent, AIState* state, float elapsedTime);
    };

    /**
     * Constructor - requires FactoryKey, use AIState::create() instead.
     *
     * @param key Factory key (only obtainable via create())
     * @param id The ID of the new AIState.
     * @script{create}
     */
    AIState(FactoryKey key, const std::string& id);

    /**
     * Destructor.
     */
    ~AIState() = default;

    // Note: create() is inherited from StandaloneCreatableWithKey<AIState>
    // Usage: auto state = AIState::create("myStateId");

    /**
     * Returns the ID of this state.
     *
     * @return The state ID.
     */
    const std::string& getId() const noexcept { return _id; }

    /**
     * Sets a listener to dispatch state events to.
     *
     * @param listener Listener to dispatch state events to, or nullptr to disable event dispatching.
     */
    void setListener(Listener* listener);

  private:
    /**
     * Hidden copy constructor.
     */
    AIState(const AIState&);

    /**
     * Hidden copy assignment operator.
     */
    AIState& operator=(const AIState&) = delete;

    /**
     * Called by AIStateMachine when this state is being entered.
     */
    void enter(AIStateMachine* stateMachine);

    /**
     * Called by AIStateMachine when this state is being exited.
     */
    void exit(AIStateMachine* stateMachine);

    /**
     * Called by AIStateMachine once per frame to update this state when it is active.
     */
    void update(AIStateMachine* stateMachine, float elapsedTime);

    std::string _id;
    Listener* _listener;

    // The default/empty state.
    static AIStatePtr _empty;
};

} // namespace tractor
