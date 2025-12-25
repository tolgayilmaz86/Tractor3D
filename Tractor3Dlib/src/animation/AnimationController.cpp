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

#include "animation/AnimationController.h"

#include "framework/Game.h"
#include "graphics/Curve.h"

namespace tractor
{

//----------------------------------------------------------------------------
void AnimationController::stopAllAnimations()
{
    for (auto& clip : _runningClips)
    {
        assert(clip);
        clip->stop();
    }
}

//----------------------------------------------------------------------------
void AnimationController::finalize()
{
    _runningClips.clear();
    _state = STOPPED;
}

//----------------------------------------------------------------------------
void AnimationController::resume()
{
    if (_runningClips.empty())
        _state = IDLE;
    else
        _state = RUNNING;
}

//----------------------------------------------------------------------------
void AnimationController::schedule(AnimationClip* clip)
{
    if (_runningClips.empty())
    {
        _state = RUNNING;
    }

    assert(clip);
    _runningClips.push_back(clip->shared_from_this());
}

//----------------------------------------------------------------------------
void AnimationController::unschedule(AnimationClip* clip)
{
    auto clipItr = std::find_if(_runningClips.begin(),
                                 _runningClips.end(),
                                 [clip](const AnimationClipPtr& rClip) { return rClip.get() == clip; });
    
    if (clipItr != _runningClips.end())
    {
        _runningClips.erase(clipItr);
    }

    if (_runningClips.empty()) _state = IDLE;
}

//----------------------------------------------------------------------------
void AnimationController::update(float elapsedTime)
{
    if (_state != RUNNING) return;

    Transform::suspendTransformChanged();

    // Loop through running clips and call update() on them.
    auto clipIter = _runningClips.begin();
    while (clipIter != _runningClips.end())
    {
        AnimationClipPtr clip = *clipIter;
        assert(clip);
        
        if (clip->isClipStateBitSet(AnimationClip::CLIP_IS_RESTARTED_BIT))
        { 
            // If the CLIP_IS_RESTARTED_BIT is set, we should end the clip and
            // move it from where it is in the running clips list to the back.
            clip->onEnd();
            clip->setClipStateBit(AnimationClip::CLIP_IS_PLAYING_BIT);
            _runningClips.push_back(clip);
            clipIter = _runningClips.erase(clipIter);
        }
        else if (clip->update(elapsedTime))
        {
            clipIter = _runningClips.erase(clipIter);
        }
        else
        {
            clipIter++;
        }
    }

    Transform::resumeTransformChanged();

    if (_runningClips.empty()) _state = IDLE;
}

} // namespace tractor
