#include "pch.h"

#include "animation/AnimationValue.h"

namespace tractor
{

AnimationValue::AnimationValue(unsigned int componentCount)
    : _componentCount(componentCount), _componentSize(componentCount * sizeof(float))
{
    assert(_componentCount > 0);
    _value = new float[_componentCount];
}

AnimationValue::AnimationValue(const AnimationValue& copy)
{
    _value = new float[copy._componentCount];
    _componentSize = copy._componentSize;
    _componentCount = copy._componentCount;
    memcpy(_value, copy._value, _componentSize);
}

AnimationValue::~AnimationValue() { SAFE_DELETE_ARRAY(_value); }

AnimationValue& AnimationValue::operator=(const AnimationValue& v)
{
    if (this != &v)
    {
        if (_value == nullptr || _componentSize != v._componentSize
            || _componentCount != v._componentCount)
        {
            _componentSize = v._componentSize;
            _componentCount = v._componentCount;
            SAFE_DELETE_ARRAY(_value);
            _value = new float[v._componentCount];
        }
        memcpy(_value, v._value, _componentSize);
    }
    return *this;
}

float AnimationValue::getFloat(unsigned int index) const
{
    assert(index < _componentCount);
    assert(_value);

    return _value[index];
}

void AnimationValue::setFloat(unsigned int index, float value)
{
    assert(index < _componentCount);
    assert(_value);

    _value[index] = value;
}

void AnimationValue::getFloats(unsigned int index, float* values, unsigned int count) const
{
    assert(_value && values && index < _componentCount && (index + count) <= _componentCount);

    memcpy(values, &_value[index], count * sizeof(float));
}

void AnimationValue::setFloats(unsigned int index, float* values, unsigned int count)
{
    assert(_value && values && index < _componentCount && (index + count) <= _componentCount);

    memcpy(&_value[index], values, count * sizeof(float));
}

} // namespace tractor
