#include "pch.h"

#include "ai/AIStateMachine.h"

#include "ai/AIAgent.h"
#include "ai/AIMessage.h"
#include "framework/Game.h"

namespace tractor
{

AIStateMachine::AIStateMachine(AIAgent* agent) : _agent(agent)
{
    assert(agent);
    if (AIState::_empty)
        AIState::_empty->addRef();
    else
        AIState::_empty = new AIState("");
    _currentState = AIState::_empty;
}

AIStateMachine::~AIStateMachine()
{
    // Release all states
    for (AIState* state : _states)
    {
        state->release();
    }

    if (AIState::_empty)
    {
        if (AIState::_empty->getRefCount() == 1)
        {
            SAFE_RELEASE(AIState::_empty);
        }
        else
        {
            AIState::_empty->release();
        }
    }
}

AIState* AIStateMachine::addState(const std::string& id)
{
    return _states.emplace_back(AIState::create(id));
}

void AIStateMachine::addState(AIState* state)
{
    state->addRef();
    _states.push_back(state);
}

void AIStateMachine::removeState(AIState* state)
{
    std::list<AIState*>::iterator itr = std::find(_states.begin(), _states.end(), state);
    if (itr != _states.end())
    {
        _states.erase(itr);
        state->release();
    }
}

AIState* AIStateMachine::getState(const std::string& id) const noexcept
{
    for (AIState* state : _states)
    {
        if (state->getId() == std::string_view{ id })
            return state;
    }

    return nullptr;
}

bool AIStateMachine::hasState(AIState* state) const
{
    assert(state);

    return (std::find(_states.begin(), _states.end(), state) != _states.end());
}

AIState* AIStateMachine::setState(const std::string& id)
{
    AIState* state = getState(id);
    if (state) sendChangeStateMessage(state);
    return state;
}

bool AIStateMachine::setState(AIState* state)
{
    if (hasState(state))
    {
        sendChangeStateMessage(state);
        return true;
    }

    return false;
}

void AIStateMachine::sendChangeStateMessage(AIState* newState)
{
    AIMessage* message = AIMessage::create(0, _agent->getId(), _agent->getId(), 1);
    message->_messageType = AIMessage::MESSAGE_TYPE_STATE_CHANGE;
    message->setString(0, newState->getId());
    Game::getInstance()->getAIController()->sendMessage(message);
}

void AIStateMachine::setStateInternal(AIState* state)
{
    assert(hasState(state));

    // Fire the exit event for the current state
    _currentState->exit(this);

    // Set the new state
    _currentState = state;

    // Fire the enter event for the new state
    _currentState->enter(this);
}

void AIStateMachine::update(float elapsedTime) { _currentState->update(this, elapsedTime); }

} // namespace tractor
