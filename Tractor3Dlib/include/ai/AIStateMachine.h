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
#include <list>

#include "ai/AIState.h"

namespace tractor
{

/**
 * Defines a simple AI state machine that can be used to program logic
 * for an AIAgent in a game.
 *
 * A state machine uses AIState objects to represent different states
 * of an object in the game. The state machine provides access to the
 * current state of an AI agent and it controls state changes as well.
 * When a new state is set, the stateExited event will be called for the
 * previous state, the stateEntered event will be called for the new state
 * and then the stateUpdate event will begin to be called each frame
 * while the new state is active.
 *
 * Communication of state changes is facilitated through the AIMessage class.
 * Messages are dispatched by the AIController and can be used for purposes
 * other than state changes as well. Messages may be sent to the state
 * machines of any other agents in a game and can contain any arbitrary
 * information. This mechanism provides a simple, flexible and easily
 * debuggable method for communicating between AI objects in a game.
 */
class AIStateMachine
{
    friend class AIAgent;
    friend class AIState;

  public:
    /**
     * Constructor.
     */
    AIStateMachine(AIAgent* agent);

    /**
     * Destructor.
     */
    ~AIStateMachine();

    /**
     * Returns the AIAgent that owns this state machine.
     *
     * @return The AIAgent that owns this state machine.
     */
    AIAgent* getAgent() const noexcept { return _agent; }

    /**
     * Creates and adds a new state to the state machine.
     *
     * @param id ID of the new state.
     *
     * @return The newly created and added state.
     */
    AIState* addState(const std::string& id);

    /**
     * Adds a state to the state machine.
     *
     * The specified state may be shared by other state machines.
     * Its reference count is increased while it is held by
     * this state machine.
     *
     * @param state The state to add.
     */
    void addState(AIState* state);

    /**
     * Removes a state from the state machine.
     *
     * @param state The state to remove.
     */
    void removeState(AIState* state);

    /**
     * Returns a state registered with this state machine.
     *
     * @param id The ID of the state to return.
     *
     * @return The state with the given ID, or nullptr if no such state exists.
     */
    AIState* getState(const std::string& id) const noexcept;

    /**
     * Returns the active state for this state machine.
     *
     * @return The active state for this state machine.
     */
    AIState* getActiveState() const noexcept { return _currentState; }

    /**
     * Changes the state of this state machine to the given state.
     *
     * If no state with the given ID exists within this state machine,
     * this method does nothing.
     *
     * @param id The ID of the new state.
     *
     * @return The new state, or nullptr if no matching state could be found.
     */
    AIState* setState(const std::string& id);

    /**
     * Changes the state of this state machine to the given state.
     *
     * If the given state is not registered with this state machine,
     * this method does nothing.
     *
     * @param state The new state.
     *
     * @return true if the state is successfully changed, false otherwise.
     */
    bool setState(AIState* state);

  private:
    /**
     * Hidden copy constructor.
     */
    AIStateMachine(const AIStateMachine&);

    /**
     * Hidden copy assignment operator.
     */
    AIStateMachine& operator=(const AIStateMachine&) = delete;

    /**
     * Sends a message to change the state of this state machine.
     */
    void sendChangeStateMessage(AIState* newState);

    /**
     * Changes the active state of the state machine.
     */
    void setStateInternal(AIState* state);

    /**
     * Determines if the specified state exists within this state machine.
     */
    bool hasState(AIState* state) const;

    /**
     * Called by AIController to update the state machine each frame.
     */
    void update(float elapsedTime);

    AIAgent* _agent;
    AIState* _currentState;
    std::list<AIState*> _states;
};

} // namespace tractor
