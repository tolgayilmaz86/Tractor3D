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

#include "animation/Animation.h"
#include "animation/AnimationClip.h"
#include "animation/AnimationTarget.h"
#include "scene/Properties.h"

namespace tractor
{

/**
 * Defines a class for controlling game animation.
 */
class AnimationController
{
    friend class Game;
    friend class Animation;
    friend class AnimationClip;
    friend class SceneLoader;

  public:
    /**
     * Constructor.
     */
    AnimationController() = default;

    /**
     * Destructor.
     */
    ~AnimationController() = default;

    /**
     * Stops all AnimationClips currently playing on the AnimationController.
     */
    void stopAllAnimations();

  private:
    /**
     * The states that the AnimationController may be in.
     */
    enum State
    {
        RUNNING,
        IDLE,
        PAUSED,
        STOPPED
    };

    /**
     * Constructor.
     */
    AnimationController(const AnimationController& copy);

    /**
     * Gets the controller's state.
     *
     * @return The current state.
     */
    AnimationController::State getState() const noexcept { return _state; }

    /**
     * Callback for when the controller is initialized.
     */
    void initialize() noexcept { _state = IDLE; }

    /*
     * Callback for when the controller is finalized.
     */
    void finalize();

    /**
     * Resumes the AnimationController.
     */
    void resume();

    /**
     * Pauses the AnimationController.
     */
    void pause() { _state = PAUSED; }

    /**
     * Schedules an AnimationClip to run.
     */
    void schedule(AnimationClip* clip);

    /**
     * Unschedules an AnimationClip.
     */
    void unschedule(AnimationClip* clip);

    /**
     * Callback for when the controller receives a frame update event.
     */
    void update(float elapsedTime);

    State _state{ STOPPED };                   // The current state of the AnimationController.
    std::list<AnimationClip*> _runningClips{}; // A list of running AnimationClips.
};

} // namespace tractor
