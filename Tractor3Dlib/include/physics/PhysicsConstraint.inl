#include "physics/PhysicsConstraint.h"

namespace tractor
{

  inline float PhysicsConstraint::getBreakingImpulse() const
  {
    assert(_constraint);
    return _constraint->getBreakingImpulseThreshold();
  }

  inline void PhysicsConstraint::setBreakingImpulse(float impulse)
  {
    assert(_constraint);
    _constraint->setBreakingImpulseThreshold(impulse);
  }

  inline bool PhysicsConstraint::isEnabled() const
  {
    assert(_constraint);
    return _constraint->isEnabled();
  }

  inline void PhysicsConstraint::setEnabled(bool enabled)
  {
    assert(_constraint);
    _constraint->setEnabled(enabled);
  }

}
