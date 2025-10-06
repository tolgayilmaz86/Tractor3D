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

#include "ai/AIState.h"

#include "ai/AIAgent.h"
#include "ai/AIStateMachine.h"
#include "scene/Node.h"

namespace tractor
{

AIState* AIState::_empty = nullptr;

AIState::AIState(const std::string& id) : _id(id), _listener(nullptr) {}

AIState* AIState::create(const std::string& id) { return new AIState(id); }

void AIState::setListener(Listener* listener) { _listener = listener; }

void AIState::enter(AIStateMachine* stateMachine)
{
    if (_listener) _listener->stateEnter(stateMachine->getAgent(), this);

    Node* node = stateMachine->_agent->_node;
    if (node)
        node->fireScriptEvent<void>(GP_GET_SCRIPT_EVENT(Node, stateEnter),
                                    dynamic_cast<void*>(node),
                                    this);
}

void AIState::exit(AIStateMachine* stateMachine)
{
    if (_listener) _listener->stateExit(stateMachine->getAgent(), this);

    Node* node = stateMachine->_agent->_node;
    if (node)
        node->fireScriptEvent<void>(GP_GET_SCRIPT_EVENT(Node, stateExit),
                                    dynamic_cast<void*>(node),
                                    this);
}

void AIState::update(AIStateMachine* stateMachine, float elapsedTime)
{
    if (_listener) _listener->stateUpdate(stateMachine->getAgent(), this, elapsedTime);

    Node* node = stateMachine->_agent->_node;
    if (node)
        node->fireScriptEvent<void>(GP_GET_SCRIPT_EVENT(Node, stateUpdate),
                                    dynamic_cast<void*>(node),
                                    this,
                                    elapsedTime);
}

AIState::Listener::~Listener() {}

void AIState::Listener::stateEnter(AIAgent* agent, AIState* state) {}

void AIState::Listener::stateExit(AIAgent* agent, AIState* state) {}

void AIState::Listener::stateUpdate(AIAgent* agent, AIState* state, float elapsedTime) {}

} // namespace tractor
