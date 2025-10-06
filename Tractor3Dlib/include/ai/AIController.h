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

#include "ai/AIAgent.h"
#include "ai/AIMessage.h"

namespace tractor
{

/**
 * Defines and facilitates the state machine execution and message passing
 * between AI objects in the game. This class is generally not interfaced
 * with directly.
 */
class AIController
{
    friend class Game;
    friend class Node;

  public:
    /**
     * Routes the specified message to its intended recipient(s).
     *
     * Messages are arbitrary packets of data that are sent either to a single or to multiple
     * recipients in the game.
     *
     * Once the specified message has been delivered, it is automatically destroyed by the
     * AIController. For this reason, AIMessage pointers should NOT be held or explicitly destroyed
     * by any code after they are sent through the AIController.
     *
     * @param message The message to send.
     * @param delay The delay (in milliseconds) to wait before sending the message.
     */
    void sendMessage(AIMessage* message, float delay = 0);

    /**
     * Searches for an AIAgent that is registered with the AIController with the specified ID.
     *
     * @param id ID of the agent to find.
     *
     * @return The first agent matching the specified ID, or nullptr if no matching agent could be found.
     */
    AIAgent* findAgent(const std::string& id) const;

    /**
     * Constructor.
     */
    AIController() = default;

    /**
     * Destructor.
     */
    ~AIController() = default;

  private:
    /**
     * Hidden copy constructor.
     */
    AIController(const AIController&);

    /**
     * Hidden copy assignment operator.
     */
    AIController& operator=(const AIController&) = delete;

    /**
     * Called during startup to initialize the AIController.
     */
    void initialize();

    /**
     * Called during shutdown to finalize the AIController.
     */
    void finalize();

    /**
     * Pauses the AIController.
     */
    void pause() { _paused = true; }


    /**
     * Resumes the AIController.
     */
    void resume() { _paused = false; }

    /**
     * Called each frame to update the AIController.
     *
     * @param elapsedTime The elapsed time, in milliseconds.
     */
    void update(float elapsedTime);

    void addAgent(AIAgent* agent);

    void removeAgent(AIAgent* agent);

    bool _paused{ false };
    AIMessage* _firstMessage{ nullptr };
    AIAgent* _firstAgent{ nullptr };
};

} // namespace tractor
