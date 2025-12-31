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

#include "animation/Animation.h"

#include "animation/AnimationClip.h"
#include "animation/AnimationController.h"
#include "animation/AnimationTarget.h"
#include "framework/Game.h"
#include "math/Transform.h"
#include "scene/Properties.h"

constexpr auto ANIMATION_INDEFINITE_STR = "INDEFINITE";
constexpr auto ANIMATION_DEFAULT_CLIP = 0;
constexpr auto ANIMATION_ROTATE_OFFSET = 0;
constexpr auto ANIMATION_SRT_OFFSET = 3;

namespace tractor
{

//----------------------------------------------------------------------------
AnimationPtr Animation::create(const std::string& id)
{
    return std::make_shared<Animation>(PrivateKey{}, id);
}

//----------------------------------------------------------------------------
Animation::Animation(PrivateKey, const std::string& id)
    : _controller(Game::getInstance()->getAnimationController()), _id(id)
{
}

//----------------------------------------------------------------------------
Animation::Animation(const std::string& id)
    : _controller(Game::getInstance()->getAnimationController()), _id(id)
{
}

//----------------------------------------------------------------------------
Animation::~Animation()
{
    // Clean up channels - remove from targets before deleting to prevent dangling pointers
    for (auto* channel : _channels)
    {
        if (channel && channel->_target)
        {
            channel->_target->removeChannel(channel);
        }
        delete channel;
    }
    _channels.clear();

    if (_defaultClip)
    {
        if (_defaultClip->isClipStateBitSet(AnimationClip::CLIP_IS_PLAYING_BIT))
        {
            assert(_controller);
            _controller->unschedule(_defaultClip.get());
        }
    }
    _defaultClip.reset();

    for (auto& [clipId, clipPtr] : _clipsMap)
    {
        if (clipPtr && clipPtr->isClipStateBitSet(AnimationClip::CLIP_IS_PLAYING_BIT))
        {
            _controller->unschedule(clipPtr.get());
        }
    }
    _clipsMap.clear();
}

//----------------------------------------------------------------------------
Animation::Channel::Channel(const std::shared_ptr<Animation>& animation,
                            AnimationTarget* target,
                            int propertyId,
                            std::shared_ptr<Curve> curve,
                            unsigned long duration)
    : _animation(animation), _target(target), _propertyId(propertyId), _curve(std::move(curve)),
      _duration(duration)
{
    assert(!_animation.expired());
    assert(_target);
    assert(_curve);

    // get property component count, and ensure the property exists on the AnimationTarget by
    // getting the property component count.
    assert(_target->getAnimationPropertyComponentCount(propertyId));
    _target->addChannel(this);
}

//----------------------------------------------------------------------------
Animation::Channel::Channel(const Channel& copy, const std::shared_ptr<Animation>& animation, AnimationTarget* target)
    : _animation(animation), _target(target), _propertyId(copy._propertyId), _curve(copy._curve),
      _duration(copy._duration)
{
    assert(_curve);
    assert(_target);
    assert(!_animation.expired());

    _target->addChannel(this);
}

//----------------------------------------------------------------------------
Animation::Channel::~Channel() = default;

//----------------------------------------------------------------------------
void Animation::createClips(const std::string& url)
{
    auto properties = std::unique_ptr<Properties>(Properties::create(url));
    assert(properties);

    Properties* pAnimation =
        properties->getNamespace().length() > 0 ? properties.get() : properties->getNextNamespace();
    assert(pAnimation);

    int frameCount = pAnimation->getInt("frameCount");
    if (frameCount <= 0) GP_ERROR("The animation's frame count must be greater than 0.");

    createClips(pAnimation, (unsigned int)frameCount);
}

//----------------------------------------------------------------------------
AnimationClip* Animation::createClip(const std::string& id, unsigned long begin, unsigned long end)
{
    auto clip = AnimationClip::create(id, this, begin, end);

    addClip(clip);

    return clip.get();
}

//----------------------------------------------------------------------------
AnimationClip* Animation::getClip(const std::string& id)
{
    // If id is nullptr return the default clip.
    if (id.empty())
    {
        if (!_defaultClip) createDefaultClip();

        return _defaultClip.get();
    }
    else
    {
        return findClip(id);
    }
}

//----------------------------------------------------------------------------
void Animation::play(const std::string& clipId)
{
    // If id is nullptr, play the default clip.
    if (clipId.empty())
    {
        if (_defaultClip == nullptr) createDefaultClip();

        _defaultClip->play();
    }
    else
    {
        // Find animation clip and play.
        AnimationClip* clip = findClip(clipId);
        if (clip != nullptr) clip->play();
    }
}

//----------------------------------------------------------------------------
void Animation::stop(const std::string& clipId)
{
    // If id is nullptr, play the default clip.
    if (clipId.empty())
    {
        if (_defaultClip) _defaultClip->stop();
    }
    else
    {
        // Find animation clip and play.
        AnimationClip* clip = findClip(clipId);
        if (clip != nullptr) clip->stop();
    }
}

//----------------------------------------------------------------------------
void Animation::pause(const std::string& clipId)
{
    if (clipId.empty())
    {
        if (_defaultClip) _defaultClip->pause();
    }
    else
    {
        AnimationClip* clip = findClip(clipId);
        if (clip != nullptr) clip->pause();
    }
}

//----------------------------------------------------------------------------
bool Animation::targets(AnimationTarget* target) const
{
    bool targetExists = std::any_of(_channels.begin(),
                                    _channels.end(),
                                    [target](const Animation::Channel* channel)
                                    {
                                        assert(channel); // Ensure channel is not null
                                        return channel->_target == target;
                                    });

    return targetExists;
}

//----------------------------------------------------------------------------
void Animation::createDefaultClip()
{
    _defaultClip = AnimationClip::create("default_clip", this, 0.0f, _duration);
}

//----------------------------------------------------------------------------
void Animation::createClips(Properties* animationProperties, unsigned int frameCount)
{
    assert(animationProperties);

    Properties* pClip = animationProperties->getNextNamespace();

    while (pClip != nullptr && pClip->getNamespace() == "clip")
    {
        int begin = pClip->getInt("begin");
        int end = pClip->getInt("end");

        AnimationClip* clip = createClip(pClip->getId(),
                                         ((float)begin / frameCount) * _duration,
                                         ((float)end / frameCount) * _duration);

        auto repeat = pClip->getString("repeatCount");
        if (!repeat.empty())
        {
            if (repeat == ANIMATION_INDEFINITE_STR)
            {
                clip->setRepeatCount(AnimationClip::REPEAT_INDEFINITE);
            }
            else
            {
                float value;
                sscanf(repeat.c_str(), "%f", &value);
                clip->setRepeatCount(value);
            }
        }

        auto speed = pClip->getString("speed");
        if (!speed.empty())
        {
            float value;
            sscanf(speed.c_str(), "%f", &value);
            clip->setSpeed(value);
        }

        clip->setLoopBlendTime(pClip->getFloat("loopBlendTime")); // returns zero if not specified

        pClip = animationProperties->getNextNamespace();
    }
}

//----------------------------------------------------------------------------
void Animation::addClip(const AnimationClipPtr& clip) { _clipsMap.insert({ clip->getId(), clip }); }

//----------------------------------------------------------------------------
AnimationClip* Animation::findClip(const std::string& id) const
{
    if (auto clip = _clipsMap.find(id); clip != _clipsMap.end()) return clip->second.get();

    return nullptr;
}

//----------------------------------------------------------------------------
Animation::Channel* Animation::createChannel(AnimationTarget* target,
                                             int propertyId,
                                             unsigned int keyCount,
                                             unsigned int* keyTimes,
                                             float* keyValues,
                                             unsigned int type)
{
    assert(target);
    assert(keyTimes);
    assert(keyValues);

    unsigned int propertyComponentCount = target->getAnimationPropertyComponentCount(propertyId);
    assert(propertyComponentCount > 0);

    auto curve = Curve::create(keyCount, propertyComponentCount);
    assert(curve);
    if (target->_targetType == AnimationTarget::TRANSFORM)
        setTransformRotationOffset(curve.get(), propertyId);

    unsigned int lowest = keyTimes[0];
    unsigned long duration = keyTimes[keyCount - 1] - lowest;

    std::vector<float> normalizedKeyTimes(keyCount);

    normalizedKeyTimes[0] = 0.0f;
    curve->setPoint(0, normalizedKeyTimes[0], keyValues, (Curve::InterpolationType)type);

    unsigned int pointOffset = propertyComponentCount;
    unsigned int i = 1;
    for (; i < keyCount - 1; i++)
    {
        normalizedKeyTimes[i] = (float)(keyTimes[i] - lowest) / (float)duration;
        curve->setPoint(i,
                        normalizedKeyTimes[i],
                        (keyValues + pointOffset),
                        (Curve::InterpolationType)type);
        pointOffset += propertyComponentCount;
    }
    if (keyCount > 1)
    {
        i = keyCount - 1;
        normalizedKeyTimes[i] = 1.0f;
        curve->setPoint(i,
                        normalizedKeyTimes[i],
                        keyValues + pointOffset,
                        (Curve::InterpolationType)type);
    }

    Channel* channel = new Channel(shared_from_this(), target, propertyId, curve, duration);
    addChannel(channel);
    return channel;
}

//----------------------------------------------------------------------------
Animation::Channel* Animation::createChannel(AnimationTarget* target,
                                             int propertyId,
                                             unsigned int keyCount,
                                             unsigned int* keyTimes,
                                             float* keyValues,
                                             float* keyInValue,
                                             float* keyOutValue,
                                             unsigned int type)
{
    assert(target);
    assert(keyTimes);
    assert(keyValues);

    unsigned int propertyComponentCount = target->getAnimationPropertyComponentCount(propertyId);
    assert(propertyComponentCount > 0);

    auto curve = Curve::create(keyCount, propertyComponentCount);
    assert(curve);
    if (target->_targetType == AnimationTarget::TRANSFORM)
        setTransformRotationOffset(curve.get(), propertyId);

    unsigned long lowest = keyTimes[0];
    unsigned long duration = keyTimes[keyCount - 1] - lowest;

    auto normalizedKeyTimes = std::make_unique<float[]>(keyCount);

    normalizedKeyTimes[0] = 0.0f;
    curve->setPoint(0,
                    normalizedKeyTimes[0],
                    keyValues,
                    (Curve::InterpolationType)type,
                    keyInValue,
                    keyOutValue);

    unsigned int pointOffset = propertyComponentCount;
    unsigned int i = 1;
    for (; i < keyCount - 1; i++)
    {
        normalizedKeyTimes[i] = (float)(keyTimes[i] - lowest) / (float)duration;
        curve->setPoint(i,
                        normalizedKeyTimes[i],
                        (keyValues + pointOffset),
                        (Curve::InterpolationType)type,
                        (keyInValue + pointOffset),
                        (keyOutValue + pointOffset));
        pointOffset += propertyComponentCount;
    }
    i = keyCount - 1;
    normalizedKeyTimes[i] = 1.0f;
    curve->setPoint(i,
                    normalizedKeyTimes[i],
                    keyValues + pointOffset,
                    (Curve::InterpolationType)type,
                    keyInValue + pointOffset,
                    keyOutValue + pointOffset);

    Channel* channel = new Channel(shared_from_this(), target, propertyId, curve, duration);
    addChannel(channel);
    return channel;
}

//----------------------------------------------------------------------------
void Animation::addChannel(Channel* channel)
{
    assert(channel);
    _channels.push_back(channel);

    if (channel->_duration > _duration) _duration = channel->_duration;
}

//----------------------------------------------------------------------------
void Animation::removeChannel(Channel* channel)
{
    std::vector<Animation::Channel*>::iterator itr = _channels.begin();
    while (itr != _channels.end())
    {
        Animation::Channel* chan = *itr;
        if (channel == chan)
        {
            _channels.erase(itr);
            return;
        }
        else
        {
            itr++;
        }
    }
}

//----------------------------------------------------------------------------
void Animation::setTransformRotationOffset(Curve* curve, unsigned int propertyId)
{
    assert(curve);

    switch (propertyId)
    {
        case Transform::ANIMATE_ROTATE:
        case Transform::ANIMATE_ROTATE_TRANSLATE:
            curve->setQuaternionOffset(ANIMATION_ROTATE_OFFSET);
            return;
        case Transform::ANIMATE_SCALE_ROTATE:
        case Transform::ANIMATE_SCALE_ROTATE_TRANSLATE:
            curve->setQuaternionOffset(ANIMATION_SRT_OFFSET);
            return;
    }

    return;
}

//----------------------------------------------------------------------------
AnimationPtr Animation::clone(Channel* channel, AnimationTarget* target)
{
    assert(channel);

    auto animation = Animation::create(getId());

    Animation::Channel* channelCopy = new Animation::Channel(*channel, animation, target);
    animation->addChannel(channelCopy);

    // Clone the clips
    if (_defaultClip) animation->_defaultClip = _defaultClip->clone(animation.get());

    for (const auto& [clipId, clipPtr] : _clipsMap)
        animation->addClip(clipPtr->clone(animation.get()));

    return animation;
}

} // namespace tractor
