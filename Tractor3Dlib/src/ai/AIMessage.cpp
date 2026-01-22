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

#include "ai/AIMessage.h"

namespace tractor
{

//----------------------------------------------------------------------------
AIMessage* AIMessage::create(unsigned int id,
                             const std::string& sender,
                             const std::string& receiver,
                             unsigned int parameterCount)
{
    AIMessage* message = new AIMessage();
    message->_id = id;
    message->_sender = sender;
    message->_receiver = receiver;
    message->_parameterCount = parameterCount;
    if (parameterCount > 0) message->_parameters = std::make_unique<AIMessage::ParameterValue[]>(parameterCount);
    return message;
}

//----------------------------------------------------------------------------
void AIMessage::destroy(AIMessage* message)
{
    delete message;
}

//----------------------------------------------------------------------------
int AIMessage::getInt(unsigned int index) const
{
    assert(index < _parameterCount);
    assert(std::holds_alternative<int>(_parameters[index]));

    return std::get<int>(_parameters[index]);
}

//----------------------------------------------------------------------------
void AIMessage::setInt(unsigned int index, int value)
{
    assert(index < _parameterCount);

    _parameters[index] = value;
}

//----------------------------------------------------------------------------
long AIMessage::getLong(unsigned int index) const
{
    assert(index < _parameterCount);
    assert(std::holds_alternative<long>(_parameters[index]));

    return std::get<long>(_parameters[index]);
}

//----------------------------------------------------------------------------
void AIMessage::setLong(unsigned int index, long value)
{
    assert(index < _parameterCount);

    _parameters[index] = value;
}

//----------------------------------------------------------------------------
float AIMessage::getFloat(unsigned int index) const
{
    assert(index < _parameterCount);
    assert(std::holds_alternative<float>(_parameters[index]));

    return std::get<float>(_parameters[index]);
}

//----------------------------------------------------------------------------
void AIMessage::setFloat(unsigned int index, float value)
{
    assert(index < _parameterCount);

    _parameters[index] = value;
}

//----------------------------------------------------------------------------
double AIMessage::getDouble(unsigned int index) const
{
    assert(index < _parameterCount);
    assert(std::holds_alternative<double>(_parameters[index]));

    return std::get<double>(_parameters[index]);
}

//----------------------------------------------------------------------------
void AIMessage::setDouble(unsigned int index, double value)
{
    assert(index < _parameterCount);

    _parameters[index] = value;
}

//----------------------------------------------------------------------------
bool AIMessage::getBoolean(unsigned int index) const
{
    assert(index < _parameterCount);
    assert(std::holds_alternative<bool>(_parameters[index]));

    return std::get<bool>(_parameters[index]);
}

//----------------------------------------------------------------------------
void AIMessage::setBoolean(unsigned int index, bool value)
{
    assert(index < _parameterCount);

    _parameters[index] = value;
}

//----------------------------------------------------------------------------
const std::string& AIMessage::getString(unsigned int index) const
{
    assert(index < _parameterCount);
    assert(std::holds_alternative<std::string>(_parameters[index]));

    return std::get<std::string>(_parameters[index]);
}

//----------------------------------------------------------------------------
void AIMessage::setString(unsigned int index, const std::string& value)
{
    assert(index < _parameterCount);

    _parameters[index] = value;
}

//----------------------------------------------------------------------------
AIMessage::ParameterType AIMessage::getParameterType(unsigned int index) const
{
    assert(index < _parameterCount);

    return indexToType(_parameters[index].index());
}

//----------------------------------------------------------------------------
void AIMessage::clearParameter(unsigned int index)
{
    assert(index < _parameterCount);

    _parameters[index] = std::monostate{};
}

//----------------------------------------------------------------------------
AIMessage::ParameterType AIMessage::indexToType(size_t index) noexcept
{
    // Variant indices match the order in ParameterValue definition:
    // 0: std::monostate (UNDEFINED)
    // 1: int (INTEGER)
    // 2: long (LONG)
    // 3: float (FLOAT)
    // 4: double (DOUBLE)
    // 5: bool (BOOLEAN)
    // 6: std::string (STRING)
    static constexpr ParameterType typeMap[] = {
        UNDEFINED,  // index 0: std::monostate
        INTEGER,    // index 1: int
        LONG,       // index 2: long
        FLOAT,      // index 3: float
        DOUBLE,     // index 4: double
        BOOLEAN,    // index 5: bool
        STRING      // index 6: std::string
    };
    
    return (index < std::size(typeMap)) ? typeMap[index] : UNDEFINED;
}

} // namespace tractor
