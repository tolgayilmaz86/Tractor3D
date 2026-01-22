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
        AIState::_empty = AIState::create("");
    _currentState = AIState::_empty;
}

//----------------------------------------------------------------------------
AIStatePtr AIStateMachine::addState(const std::string& id)
{
    auto state = AIState::create(id);
    _states.push_back(state);
    return state;
}

//----------------------------------------------------------------------------
void AIStateMachine::addState(AIStatePtr state)
{
    _states.push_back(state);
}

//----------------------------------------------------------------------------
void AIStateMachine::removeState(AIStatePtr state)
{
    auto itr = std::find(_states.begin(), _states.end(), state);
    if (itr != _states.end())
    {
        _states.erase(itr);
    }
}

//----------------------------------------------------------------------------
AIStatePtr AIStateMachine::getState(const std::string& id) const noexcept
{
    for (const auto& state : _states)
    {
        if (state->getId() == std::string_view{ id })
            return state;
    }

    return nullptr;
}

//----------------------------------------------------------------------------
bool AIStateMachine::hasState(AIStatePtr state) const
{
    assert(state);

    return (std::find(_states.begin(), _states.end(), state) != _states.end());
}

//----------------------------------------------------------------------------
AIStatePtr AIStateMachine::setState(const std::string& id)
{
    auto state = getState(id);
    if (state) sendChangeStateMessage(state);
    return state;
}

//----------------------------------------------------------------------------
bool AIStateMachine::setState(AIStatePtr state)
{
    if (hasState(state))
    {
        sendChangeStateMessage(state);
        return true;
    }

    return false;
}

//----------------------------------------------------------------------------
void AIStateMachine::sendChangeStateMessage(AIStatePtr newState)
{
    AIMessage* message = AIMessage::create(0, _agent->getId(), _agent->getId(), 1);
    message->_messageType = AIMessage::MESSAGE_TYPE_STATE_CHANGE;
    message->setString(0, newState->getId());
    Game::getInstance()->getAIController()->sendMessage(message);
}

//----------------------------------------------------------------------------
void AIStateMachine::setStateInternal(AIStatePtr state)
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
