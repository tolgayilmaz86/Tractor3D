#include "pch.h"

#include "ai/AIMessage.h"

namespace tractor
{

	AIMessage::AIMessage()
		: _id(0), _deliveryTime(0), _parameters(nullptr), _parameterCount(0), _messageType(MESSAGE_TYPE_CUSTOM), _next(nullptr)
	{
	}

	AIMessage::~AIMessage()
	{
		SAFE_DELETE_ARRAY(_parameters);
	}

	AIMessage* AIMessage::create(unsigned int id, const std::string& sender, const std::string& receiver, unsigned int parameterCount)
	{
		AIMessage* message = new AIMessage();
		message->_id = id;
		message->_sender = sender;
		message->_receiver = receiver;
		message->_parameterCount = parameterCount;
		if (parameterCount > 0)
			message->_parameters = new AIMessage::Parameter[parameterCount];
		return message;
	}

	void AIMessage::destroy(AIMessage* message)
	{
		SAFE_DELETE(message);
	}

	unsigned int AIMessage::getId() const
	{
		return _id;
	}

	const std::string& AIMessage::getSender() const
	{
		return _sender;
	}

	const std::string& AIMessage::getReceiver() const
	{
		return _receiver;
	}

	double AIMessage::getDeliveryTime() const
	{
		return _deliveryTime;
	}

	int AIMessage::getInt(unsigned int index) const
	{
		assert(index < _parameterCount);
		assert(_parameters[index].type == AIMessage::INTEGER);

		return _parameters[index].intValue;
	}

	void AIMessage::setInt(unsigned int index, int value)
	{
		assert(index < _parameterCount);

		clearParameter(index);

		_parameters[index].intValue = value;
		_parameters[index].type = AIMessage::INTEGER;
	}

	long AIMessage::getLong(unsigned int index) const
	{
		assert(index < _parameterCount);
		assert(_parameters[index].type == AIMessage::LONG);

		return _parameters[index].longValue;
	}

	void AIMessage::setLong(unsigned int index, long value)
	{
		assert(index < _parameterCount);

		clearParameter(index);

		_parameters[index].longValue = value;
		_parameters[index].type = AIMessage::LONG;
	}

	float AIMessage::getFloat(unsigned int index) const
	{
		assert(index < _parameterCount);
		assert(_parameters[index].type == AIMessage::FLOAT);

		return _parameters[index].floatValue;
	}

	void AIMessage::setFloat(unsigned int index, float value)
	{
		assert(index < _parameterCount);

		clearParameter(index);

		_parameters[index].floatValue = value;
		_parameters[index].type = AIMessage::FLOAT;
	}

	double AIMessage::getDouble(unsigned int index) const
	{
		assert(index < _parameterCount);
		assert(_parameters[index].type == AIMessage::DOUBLE);

		return _parameters[index].doubleValue;
	}

	void AIMessage::setDouble(unsigned int index, double value)
	{
		assert(index < _parameterCount);

		clearParameter(index);

		_parameters[index].doubleValue = value;
		_parameters[index].type = AIMessage::DOUBLE;
	}

	bool AIMessage::getBoolean(unsigned int index) const
	{
		assert(index < _parameterCount);
		assert(_parameters[index].type == AIMessage::BOOLEAN);

		return _parameters[index].boolValue;
	}

	void AIMessage::setBoolean(unsigned int index, bool value)
	{
		assert(index < _parameterCount);

		clearParameter(index);

		_parameters[index].boolValue = value;
		_parameters[index].type = AIMessage::BOOLEAN;
	}

	const std::string& AIMessage::getString(unsigned int index) const
	{
		assert(index < _parameterCount);
		assert(_parameters[index].type == AIMessage::STRING);

		return _parameters[index].stringValue;
	}

	void AIMessage::setString(unsigned int index, const std::string& value)
	{
		assert(index < _parameterCount);
		clearParameter(index);

		_parameters[index].stringValue = value;
		_parameters[index].type = AIMessage::STRING;
	}

	unsigned int AIMessage::getParameterCount() const
	{
		return _parameterCount;
	}

	AIMessage::ParameterType AIMessage::getParameterType(unsigned int index) const
	{
		assert(index < _parameterCount);

		return _parameters[index].type;
	}

	void AIMessage::clearParameter(unsigned int index)
	{
		assert(index < _parameterCount);

		_parameters[index].clear();
	}

	AIMessage::Parameter::Parameter()
		: type(UNDEFINED)
	{
	}

	AIMessage::Parameter::~Parameter()
	{
		clear();
	}

	void AIMessage::Parameter::clear()
	{
		if (type == AIMessage::STRING)
			stringValue.clear();

		type = AIMessage::UNDEFINED;
	}

}
