#include "pch.h"
#include "animation/AnimationTarget.h"
#include "animation/Animation.h"
#include "framework/Game.h"
#include "scene/Node.h"
#include "renderer/MaterialParameter.h"

#define ANIMATION_TARGET_INDEFINITE_STR "INDEFINITE"

namespace tractor
{

	AnimationTarget::AnimationTarget()
		: _targetType(SCALAR), _animationChannels(nullptr)
	{
	}

	AnimationTarget::~AnimationTarget()
	{
		if (_animationChannels)
		{
			std::vector<Animation::Channel*>::iterator itr = _animationChannels->begin();
			while (itr != _animationChannels->end())
			{
				Animation::Channel* channel = (*itr);
				assert(channel);
				assert(channel->_animation);
				channel->_animation->removeChannel(channel);
				SAFE_DELETE(channel);
				itr++;
			}
			_animationChannels->clear();
			SAFE_DELETE(_animationChannels);
		}
	}

	Animation* AnimationTarget::createAnimation(const std::string& id, int propertyId, unsigned int keyCount, unsigned int* keyTimes, float* keyValues, Curve::InterpolationType type)
	{
		assert(type != Curve::BEZIER && type != Curve::HERMITE);
		assert(keyCount >= 1 && keyTimes && keyValues);

		Animation* animation = new Animation(id, this, propertyId, keyCount, keyTimes, keyValues, type);

		return animation;
	}

	Animation* AnimationTarget::createAnimation(const std::string& id, int propertyId, unsigned int keyCount, unsigned int* keyTimes, float* keyValues, float* keyInValue, float* keyOutValue, Curve::InterpolationType type)
	{
		assert(keyCount >= 1 && keyTimes && keyValues && keyInValue && keyOutValue);
		Animation* animation = new Animation(id, this, propertyId, keyCount, keyTimes, keyValues, keyInValue, keyOutValue, type);

		return animation;
	}

	Animation* AnimationTarget::createAnimation(const std::string& id, const std::string& url)
	{
		Properties* p = Properties::create(url);
		assert(p);

		Animation* animation = createAnimation(id, (p->getNamespace().length() > 0) ? p : p->getNextNamespace());

		SAFE_DELETE(p);

		return animation;
	}

	Animation* AnimationTarget::createAnimationFromTo(const std::string& id, int propertyId, float* from, float* to, Curve::InterpolationType type, unsigned long duration)
	{
		assert(from);
		assert(to);

		const unsigned int propertyComponentCount = getAnimationPropertyComponentCount(propertyId);
		assert(propertyComponentCount > 0);
		float* keyValues = new float[2 * propertyComponentCount];

		memcpy(keyValues, from, sizeof(float) * propertyComponentCount);
		memcpy(keyValues + propertyComponentCount, to, sizeof(float) * propertyComponentCount);

		unsigned int* keyTimes = new unsigned int[2];
		keyTimes[0] = 0;
		keyTimes[1] = (unsigned int)duration;

		Animation* animation = createAnimation(id, propertyId, 2, keyTimes, keyValues, type);

		SAFE_DELETE_ARRAY(keyValues);
		SAFE_DELETE_ARRAY(keyTimes);

		return animation;
	}

	Animation* AnimationTarget::createAnimationFromBy(const std::string& id, int propertyId, float* from, float* by, Curve::InterpolationType type, unsigned long duration)
	{
		assert(from);
		assert(by);

		const unsigned int propertyComponentCount = getAnimationPropertyComponentCount(propertyId);
		assert(propertyComponentCount > 0);
		float* keyValues = new float[2 * propertyComponentCount];

		memcpy(keyValues, from, sizeof(float) * propertyComponentCount);

		convertByValues(propertyId, propertyComponentCount, from, by);
		memcpy(keyValues + propertyComponentCount, by, sizeof(float) * propertyComponentCount);

		unsigned int* keyTimes = new unsigned int[2];
		keyTimes[0] = 0;
		keyTimes[1] = (unsigned int)duration;

		Animation* animation = createAnimation(id, propertyId, 2, keyTimes, keyValues, type);

		SAFE_DELETE_ARRAY(keyValues);
		SAFE_DELETE_ARRAY(keyTimes);

		return animation;
	}

	Animation* AnimationTarget::createAnimation(const std::string& id, Properties* animationProperties)
	{
		assert(animationProperties);
		if (animationProperties->getNamespace() != "animation")
		{
			GP_ERROR("Invalid animation namespace '%s'.", animationProperties->getNamespace());
			return nullptr;
		}

		auto propertyIdStr = animationProperties->getString("property");
		if (propertyIdStr.empty())
		{
			GP_ERROR("Attribute 'property' must be specified for an animation.");
			return nullptr;
		}

		// Get animation target property id
		int propertyId = getPropertyId(_targetType, propertyIdStr);
		if (propertyId == -1)
		{
			GP_ERROR("Property ID is invalid.");
			return nullptr;
		}

		unsigned int keyCount = animationProperties->getInt("keyCount");
		if (keyCount == 0)
		{
			GP_ERROR("Attribute 'keyCount' must be specified for an animation.");
			return nullptr;
		}

		auto keyTimesStr = animationProperties->getString("keyTimes");
		if (keyTimesStr.empty())
		{
			GP_ERROR("Attribute 'keyTimes' must be specified for an animation.");
			return nullptr;
		}

		auto keyValuesStr = animationProperties->getString("keyValues");
		if (keyValuesStr.empty())
		{
			GP_ERROR("Attribute 'keyValues' must be specified for an animation.");
			return nullptr;
		}

		auto curveStr = animationProperties->getString("curve");
		if (curveStr.empty())
		{
			GP_ERROR("Attribute 'curve' must be specified for an animation.");
			return nullptr;
		}

		char delimeter = ' ';
		size_t startOffset = 0;
		size_t endOffset = std::string::npos;

		unsigned int* keyTimes = new unsigned int[keyCount];
		for (size_t i = 0; i < keyCount; i++)
		{
			endOffset = static_cast<std::string>(keyTimesStr).find_first_of(delimeter, startOffset);
			if (endOffset != std::string::npos)
			{
				keyTimes[i] = std::strtoul(static_cast<std::string>(keyTimesStr).substr(startOffset, endOffset - startOffset).c_str(), nullptr, 0);
			}
			else
			{
				keyTimes[i] = std::strtoul(static_cast<std::string>(keyTimesStr).substr(startOffset, static_cast<std::string>(keyTimesStr).length()).c_str(), nullptr, 0);
			}
			startOffset = endOffset + 1;
		}

		startOffset = 0;
		endOffset = std::string::npos;

		int componentCount = getAnimationPropertyComponentCount(propertyId);
		assert(componentCount > 0);

		unsigned int components = keyCount * componentCount;

		float* keyValues = new float[components];
		for (unsigned int i = 0; i < components; i++)
		{
			endOffset = static_cast<std::string>(keyValuesStr).find_first_of(delimeter, startOffset);
			if (endOffset != std::string::npos)
			{
				keyValues[i] = std::atof(static_cast<std::string>(keyValuesStr).substr(startOffset, endOffset - startOffset).c_str());
			}
			else
			{
				keyValues[i] = std::atof(static_cast<std::string>(keyValuesStr).substr(startOffset, static_cast<std::string>(keyValuesStr).length()).c_str());
			}
			startOffset = endOffset + 1;
		}

		auto keyInStr = animationProperties->getString("keyIn");
		float* keyIn = nullptr;
		if (!keyInStr.empty())
		{
			keyIn = new float[components];
			startOffset = 0;
			endOffset = std::string::npos;
			for (unsigned int i = 0; i < components; i++)
			{
				endOffset = static_cast<std::string>(keyInStr).find_first_of(delimeter, startOffset);
				if (endOffset != std::string::npos)
				{
					keyIn[i] = std::atof(static_cast<std::string>(keyInStr).substr(startOffset, endOffset - startOffset).c_str());
				}
				else
				{
					keyIn[i] = std::atof(static_cast<std::string>(keyInStr).substr(startOffset, static_cast<std::string>(keyInStr).length()).c_str());
				}
				startOffset = endOffset + 1;
			}
		}

		auto keyOutStr = animationProperties->getString("keyOut");
		float* keyOut = nullptr;
		if (!keyOutStr.empty())
		{
			keyOut = new float[components];
			startOffset = 0;
			endOffset = std::string::npos;
			for (unsigned int i = 0; i < components; i++)
			{
				endOffset = static_cast<std::string>(keyOutStr).find_first_of(delimeter, startOffset);
				if (endOffset != std::string::npos)
				{
					keyOut[i] = std::atof(static_cast<std::string>(keyOutStr).substr(startOffset, endOffset - startOffset).c_str());
				}
				else
				{
					keyOut[i] = std::atof(static_cast<std::string>(keyOutStr).substr(startOffset, static_cast<std::string>(keyOutStr).length()).c_str());
				}
				startOffset = endOffset + 1;
			}
		}

		int curve = Curve::getInterpolationType(curveStr);

		Animation* animation = nullptr;
		if (keyIn && keyOut)
		{
			animation = createAnimation(id, propertyId, keyCount, keyTimes, keyValues, keyIn, keyOut, (Curve::InterpolationType)curve);
		}
		else
		{
			animation = createAnimation(id, propertyId, keyCount, keyTimes, keyValues, (Curve::InterpolationType)curve);
		}

		auto repeat = animationProperties->getString("repeatCount");
		if (!repeat.empty())
		{
			if (repeat == ANIMATION_TARGET_INDEFINITE_STR)
			{
				animation->getClip()->setRepeatCount(AnimationClip::REPEAT_INDEFINITE);
			}
			else
			{
				float value;
				sscanf(repeat.c_str(), "%f", &value);
				animation->getClip()->setRepeatCount(value);
			}
		}

		SAFE_DELETE_ARRAY(keyOut);
		SAFE_DELETE_ARRAY(keyIn);
		SAFE_DELETE_ARRAY(keyValues);
		SAFE_DELETE_ARRAY(keyTimes);

		Properties* pClip = animationProperties->getNextNamespace();
		if (pClip && pClip->getNamespace() == "clip")
		{
			int frameCount = animationProperties->getInt("frameCount");
			if (frameCount <= 0)
			{
				GP_ERROR("Frame count must be greater than zero for a clip.");
				return animation;
			}
			animation->createClips(animationProperties, (unsigned int)frameCount);
		}

		return animation;
	}

	void AnimationTarget::destroyAnimation(const std::string& id)
	{
		// Find the animation with the specified ID.
		Animation::Channel* channel = getChannel(id);
		if (channel == nullptr)
			return;

		// Remove this target's channel from animation, and from the target's list of channels.
		assert(channel->_animation);
		channel->_animation->removeChannel(channel);
		removeChannel(channel);

		SAFE_DELETE(channel);
	}

	Animation* AnimationTarget::getAnimation(const std::string& id) const
	{
		if (_animationChannels)
		{
			std::vector<Animation::Channel*>::iterator itr = _animationChannels->begin();
			assert(*itr);

			if (id.empty())
				return (*itr)->_animation;

			for (auto& channel : *_animationChannels)
			{
				assert(channel);
				assert(channel->_animation);
				if (channel->_animation->_id.compare(id) == 0)
				{
					return channel->_animation;
				}
			}
		}

		return nullptr;
	}

	int AnimationTarget::getPropertyId(TargetType type, const std::string& propertyIdStr)
	{
		if (type == AnimationTarget::TRANSFORM)
		{
			if (propertyIdStr == "ANIMATE_SCALE")
			{
				return Transform::ANIMATE_SCALE;
			}
			else if (propertyIdStr == "ANIMATE_SCALE_X")
			{
				return Transform::ANIMATE_SCALE_X;
			}
			else if (propertyIdStr == "ANIMATE_SCALE_Y")
			{
				return Transform::ANIMATE_SCALE_Y;
			}
			else if (propertyIdStr == "ANIMATE_SCALE_Z")
			{
				return Transform::ANIMATE_SCALE_Z;
			}
			else if (propertyIdStr == "ANIMATE_ROTATE")
			{
				return Transform::ANIMATE_ROTATE;
			}
			else if (propertyIdStr == "ANIMATE_TRANSLATE")
			{
				return Transform::ANIMATE_TRANSLATE;
			}
			else if (propertyIdStr == "ANIMATE_TRANSLATE_X")
			{
				return Transform::ANIMATE_TRANSLATE_X;
			}
			else if (propertyIdStr == "ANIMATE_TRANSLATE_Y")
			{
				return Transform::ANIMATE_TRANSLATE_Y;
			}
			else if (propertyIdStr == "ANIMATE_TRANSLATE_Z")
			{
				return Transform::ANIMATE_TRANSLATE_Z;
			}
			else if (propertyIdStr == "ANIMATE_ROTATE_TRANSLATE")
			{
				return Transform::ANIMATE_ROTATE_TRANSLATE;
			}
			else if (propertyIdStr == "ANIMATE_SCALE_ROTATE_TRANSLATE")
			{
				return Transform::ANIMATE_SCALE_ROTATE_TRANSLATE;
			}
		}
		else
		{
			if (propertyIdStr == "ANIMATE_UNIFORM")
			{
				return MaterialParameter::ANIMATE_UNIFORM;
			}
		}

		return -1;
	}

	void AnimationTarget::addChannel(Animation::Channel* channel)
	{
		if (_animationChannels == nullptr)
			_animationChannels = new std::vector<Animation::Channel*>;

		assert(channel);
		_animationChannels->push_back(channel);
	}

	void AnimationTarget::removeChannel(Animation::Channel* channel)
	{
		if (_animationChannels)
		{
			auto itr = std::remove_if(_animationChannels->begin(), _animationChannels->end(),
				[channel](Animation::Channel* temp) {
					return temp == channel;
				});

			// Only erase if we found any matching channel
			if (itr != _animationChannels->end())
			{
				_animationChannels->erase(itr, _animationChannels->end());

				// Check if the vector is empty before deleting
				if (_animationChannels->empty())
				{
					SAFE_DELETE(_animationChannels);
				}
			}
		}
	}

	Animation::Channel* AnimationTarget::getChannel(const std::string& id) const
	{
		if (_animationChannels)
		{
			if (id.empty())
			{
				return _animationChannels->front(); // Return the first element if id is null
			}

			for (const auto& channelPtr : *_animationChannels)
			{

				assert(channelPtr);

				if (channelPtr->_animation->_id.compare(id) == 0)
				{
					// Found!
					return channelPtr;
				}
			}
		}

		return nullptr;
	}

	void AnimationTarget::cloneInto(AnimationTarget* target, NodeCloneContext& context) const
	{
		if (_animationChannels)
		{
			for (Animation::Channel* channel : *_animationChannels)
			{
				assert(channel);
				assert(channel->_animation);

				Animation* animation = context.findClonedAnimation(channel->_animation);
				if (animation != nullptr)
				{
					Animation::Channel* channelCopy = new Animation::Channel(*channel, animation, target);
					animation->addChannel(channelCopy);
				}
				else
				{
					// Clone the animation and register it with the context so that it only gets cloned once.
					animation = channel->_animation->clone(channel, target);
					context.registerClonedAnimation(channel->_animation, animation);
				}
			}

		}
	}

	void AnimationTarget::convertByValues(unsigned int propertyId, unsigned int componentCount, float* from, float* by)
	{
		if (_targetType == AnimationTarget::TRANSFORM)
		{
			switch (propertyId)
			{
			case Transform::ANIMATE_SCALE:
			case Transform::ANIMATE_SCALE_UNIT:
			case Transform::ANIMATE_SCALE_X:
			case Transform::ANIMATE_SCALE_Y:
			case Transform::ANIMATE_SCALE_Z:
			{
				convertScaleByValues(from, by, componentCount);
				break;
			}
			case Transform::ANIMATE_TRANSLATE:
			case Transform::ANIMATE_TRANSLATE_X:
			case Transform::ANIMATE_TRANSLATE_Y:
			case Transform::ANIMATE_TRANSLATE_Z:
			{
				convertByValues(from, by, componentCount);
				break;
			}
			case Transform::ANIMATE_ROTATE:
			{
				convertQuaternionByValues(from, by);
				break;
			}
			case Transform::ANIMATE_ROTATE_TRANSLATE:
			{
				convertQuaternionByValues(from, by);
				convertByValues(from + 4, by + 4, 3);
				break;
			}
			case Transform::ANIMATE_SCALE_ROTATE_TRANSLATE:
			{
				convertScaleByValues(from, by, 3);
				convertQuaternionByValues(from + 3, by + 3);
				convertByValues(from + 7, by + 7, 3);
				break;
			}
			}
		}
		else
		{
			convertByValues(from, by, componentCount);
		}
	}

	void AnimationTarget::convertQuaternionByValues(float* from, float* by)
	{
		Quaternion qFrom(from);
		Quaternion qBy(by);
		qBy.multiply(qFrom);
		memcpy(by, (float*)&qBy, sizeof(float) * 4);
	}

	void AnimationTarget::convertScaleByValues(float* from, float* by, unsigned int componentCount)
	{
		for (unsigned int i = 0; i < componentCount; i++)
		{
			by[i] *= from[i];
		}
	}

	void AnimationTarget::convertByValues(float* from, float* by, unsigned int componentCount)
	{
		for (unsigned int i = 0; i < componentCount; i++)
		{
			by[i] += from[i];
		}
	}

}



