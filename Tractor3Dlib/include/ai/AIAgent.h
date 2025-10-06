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

#include "ai/AIMessage.h"
#include "ai/AIStateMachine.h"
#include "utils/Ref.h"

namespace tractor
{

class Node;

/**
 * Defines an AI agent that can be added to nodes in a scene.
 *
 * Agents represent a unit of intelligence in a game and can be used
 * to program logic for a character or object in a game, using constructs
 * such as state machines. By default, an AIAgent has an empty state
 * machine.
 */
class AIAgent : public Ref
{
    friend class Node;
    friend class AIState;
    friend class AIController;

  public:
    /**
     * Interface for listening to AIAgent events.
     */
    class Listener
    {
      public:
        /**
         * Virtual destructor.
         */
        virtual ~Listener() {};

        /**
         * Called when a new message is sent to the AIAgent.
         *
         * Both global/broadcast messages and messages sent explicitly to the
         * AIAgent are sent through this method. Returning true from this method
         * will mark the message as handled and it will dispose of the message
         * and prevent any other possible recipients from receiving the message.
         * Alternatively, returning false allows the message to continue being
         * routed though the AI system.
         *
         * @param message The message received.
         *
         * @return true to mark the message as handled, false otherwise.
         */
        virtual bool messageReceived(AIMessage* message) = 0;
    };

    /**
     * Creates a new AIAgent.
     *
     * @return A new AIAgent.
     * @script{create}
     */
    static AIAgent* create();

    /**
     * Returns the identifier for the AIAgent.
     *
     * This method simply returns the ID of the Node which this AIAgent
     * is bound to. If this AIAgent is not bound to a Node, this method
     * returns an empty string.
     *
     * @return The identifier for the agent.
     */
    const std::string& getId() const;

    /**
     * Returns the Node this AIAgent is assigned to.
     *
     * @return The Node this agent is assigned to.
     */
    Node* getNode() const noexcept { return _node; }

    /**
     * Returns the state machine for the AIAgent.
     *
     * @return The agent's state machine.
     */
    AIStateMachine* getStateMachine() const noexcept { return _stateMachine.get(); }

    /**
     * Determines if this AIAgent is currently enabled.
     *
     * Agents are always disabled until they have been associated
     * with a valid Node though Node::setAgent(AIAgent*). In addition,
     * an AIAgent can be explicitly enabled or disabled using the
     * setEnabled(bool) method.
     *
     * @return true if the agent is enabled, false otherwise.
     */
    bool isEnabled() const noexcept { return (_node && _enabled); }

    /**
     * Sets whether this AIAgent is enabled.
     *
     * By default, AIAgents are enabled and they can receive messages and state
     * changes. When disabled, AIAgents stop receiving messages and their state
     * machines are halted until they are re-enabled.
     *
     * @param enabled true if the AIAgent should be enabled, false otherwise.
     */
    void setEnabled(bool enabled) noexcept { _enabled = enabled; }

    /**
     * Sets an event listener for this AIAgent.
     *
     * @param listener The new AIAgent listener, or nullptr to remove any existing listener.
     */
    void setListener(Listener* listener) noexcept { _listener = listener; }

  private:
    /**
     * Constructor.
     */
    AIAgent();

    /**
     * Destructor.
     *
     * Hidden, use SAFE_RELEASE instead.
     */
    virtual ~AIAgent() = default;

    /**
     * Hidden copy constructor.
     */
    AIAgent(const AIAgent&);

    /**
     * Hidden copy assignment operator.
     */
    AIAgent& operator=(const AIAgent&) = delete;

    /**
     * Set the node this agent is attached to.
     */
    void setNode(Node* node) noexcept { _node = node; }

    /**
     * Called by the AIController to process a message for the AIAgent.
     *
     * @param message The message to be processed.
     *
     * @return true if the message was handled, false otherwise.
     */
    bool processMessage(AIMessage* message);

    /**
     * Called once per frame by the AIController to update the agent.
     */
    void update(float elapsedTime);

    std::unique_ptr<AIStateMachine> _stateMachine;
    Node* _node{ nullptr };
    bool _enabled{ true };
    Listener* _listener{ nullptr };
    AIAgent* _next{ nullptr };
};

} // namespace tractor
