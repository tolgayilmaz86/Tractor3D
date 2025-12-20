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

#include "ai/AIController.h"

#include "framework/Game.h"

namespace tractor
{

void AIController::initialize() {}

//----------------------------------------------------------------------------
void AIController::finalize()
{
    // Clear all agents (shared_ptr handles cleanup automatically)
    _agents.clear();

    // Remove all messages
    AIMessage* message = _firstMessage;
    while (message)
    {
        AIMessage* temp = message;
        message = message->_next;
        AIMessage::destroy(temp);
    }
    _firstMessage = nullptr;
}

//----------------------------------------------------------------------------
void AIController::sendMessage(AIMessage* message, float delay)
{
    if (delay <= 0)
    {
        // Send instantly
        if (message->getReceiver().empty())
        {
            // Broadcast message to all agents
            for (const auto& agent : _agents)
            {
                if (agent->processMessage(message))
                    break; // message consumed by this agent - stop bubbling
            }
        }
        else
        {
            // Single recipient
            auto agent = findAgent(message->getReceiver());
            if (agent)
            {
                agent->processMessage(message);
            }
            else
            {
                GP_WARN("Failed to locate AIAgent for message recipient: %s", message->getReceiver());
            }
        }

        // Delete the message, since it is finished being processed
        AIMessage::destroy(message);
    }
    else
    {
        // Queue for later delivery
        if (_firstMessage) message->_next = _firstMessage;
        _firstMessage = message;
    }
}

//----------------------------------------------------------------------------
void AIController::update(float elapsedTime)
{
    if (_paused) return;

    static Game* game = Game::getInstance();

    // Send all pending messages that have expired
    AIMessage* prevMsg = nullptr;
    AIMessage* msg = _firstMessage;
    while (msg)
    {
        // If the message delivery time has expired, send it (this also deletes it)
        if (msg->getDeliveryTime() >= game->getGameTime())
        {
            // Link the message out of our list
            if (prevMsg) prevMsg->_next = msg->_next;

            AIMessage* temp = msg;
            msg = msg->_next;
            temp->_next = nullptr;
            sendMessage(temp);
        }
        else
        {
            prevMsg = msg;
            msg = msg->_next;
        }
    }

    // Update all enabled agents
    for (const auto& agent : _agents)
    {
        if (agent->isEnabled()) 
            agent->update(elapsedTime);
    }
}

//----------------------------------------------------------------------------
void AIController::addAgent(const AIAgentPtr& agent)
{
    _agents.push_back(agent);
}

//----------------------------------------------------------------------------
void AIController::removeAgent(const AIAgentPtr& agent)
{
    auto it = std::find(_agents.begin(), _agents.end(), agent);
    if (it != _agents.end())
    {
        _agents.erase(it);
    }
}

//----------------------------------------------------------------------------
AIAgentPtr AIController::findAgent(const std::string& id) const
{
    for (const auto& agent : _agents)
    {
        if (id == agent->getId()) 
            return agent;
    }

    return nullptr;
}

} // namespace tractor
