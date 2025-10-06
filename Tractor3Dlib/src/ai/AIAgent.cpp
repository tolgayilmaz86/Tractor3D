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

#include "ai/AIAgent.h"

#include "scene/Node.h"

namespace tractor
{

//----------------------------------------------------------------------------
AIAgent::AIAgent()
{
    _stateMachine = std::make_unique<AIStateMachine>(this);
}

//----------------------------------------------------------------------------
AIAgent* AIAgent::create() { return new AIAgent(); }

//----------------------------------------------------------------------------
const std::string& AIAgent::getId() const
{
    if (_node) return _node->getId();

    return EMPTY_STRING;
}

//----------------------------------------------------------------------------
void AIAgent::update(float elapsedTime) { _stateMachine->update(elapsedTime); }

//----------------------------------------------------------------------------
bool AIAgent::processMessage(AIMessage* message)
{
    // Handle built-in message types.
    switch (message->_messageType)
    {
        case AIMessage::MESSAGE_TYPE_STATE_CHANGE:
        {
            // Change state message
            const auto& stateId = message->getString(0);
            if (!stateId.empty())
            {
                AIState* state = _stateMachine->getState(stateId);
                if (state) _stateMachine->setStateInternal(state);
            }
        }
        break;
        case AIMessage::MESSAGE_TYPE_CUSTOM:
            break;
    }

    // Dispatch message to registered listener.
    if (_listener && _listener->messageReceived(message)) return true;

    if (_node
        && _node->fireScriptEvent<bool>(GP_GET_SCRIPT_EVENT(Node, messageReceived),
                                        dynamic_cast<void*>(_node),
                                        message))
        return true;

    return false;
}

} // namespace tractor
