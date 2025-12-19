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
#include "pch.h"

#include "ai/AIStateMachine.h"

#include "ai/AIAgent.h"
#include "ai/AIMessage.h"
#include "framework/Game.h"

namespace tractor
{

//----------------------------------------------------------------------------
AIStateMachine::AIStateMachine(AIAgent* agent) : _agent(agent)
{
    assert(agent);
    if (!AIState::_empty)
        AIState::_empty = std::make_shared<AIState>("");
    _currentState = AIState::_empty;
}

//----------------------------------------------------------------------------
AIStateMachine::~AIStateMachine()
{
    // shared_ptr handles cleanup automatically
    _states.clear();
    _currentState.reset();
}

//----------------------------------------------------------------------------
std::shared_ptr<AIState> AIStateMachine::addState(const std::string& id)
{
    auto state = AIState::create(id);
    _states.push_back(state);
    return state;
}

//----------------------------------------------------------------------------
void AIStateMachine::addState(std::shared_ptr<AIState> state)
{
    _states.push_back(state);
}

//----------------------------------------------------------------------------
void AIStateMachine::removeState(std::shared_ptr<AIState> state)
{
    auto itr = std::find(_states.begin(), _states.end(), state);
    if (itr != _states.end())
    {
        _states.erase(itr);
    }
}

//----------------------------------------------------------------------------
std::shared_ptr<AIState> AIStateMachine::getState(const std::string& id) const noexcept
{
    for (const auto& state : _states)
    {
        if (state->getId() == std::string_view{ id })
            return state;
    }

    return nullptr;
}

//----------------------------------------------------------------------------
bool AIStateMachine::hasState(std::shared_ptr<AIState> state) const
{
    assert(state);

    return (std::find(_states.begin(), _states.end(), state) != _states.end());
}

//----------------------------------------------------------------------------
std::shared_ptr<AIState> AIStateMachine::setState(const std::string& id)
{
    auto state = getState(id);
    if (state) sendChangeStateMessage(state);
    return state;
}

//----------------------------------------------------------------------------
bool AIStateMachine::setState(std::shared_ptr<AIState> state)
{
    if (hasState(state))
    {
        sendChangeStateMessage(state);
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------
void AIStateMachine::sendChangeStateMessage(std::shared_ptr<AIState> newState)
{
    AIMessage* message = AIMessage::create(0, _agent->getId(), _agent->getId(), 1);
    message->_messageType = AIMessage::MESSAGE_TYPE_STATE_CHANGE;
    message->setString(0, newState->getId());
    Game::getInstance()->getAIController()->sendMessage(message);
}

//----------------------------------------------------------------------------
void AIStateMachine::setStateInternal(std::shared_ptr<AIState> state)
{
    assert(hasState(state));

    // Fire the exit event for the current state
    _currentState->exit(this);

    // Set the new state
    _currentState = state;

    // Fire the enter event for the new state
    _currentState->enter(this);
}

} // namespace tractor
