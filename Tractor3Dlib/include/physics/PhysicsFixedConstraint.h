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

#include "physics/PhysicsGenericConstraint.h"

namespace tractor
{

/**
 * Defines a constraint where two rigid bodies
 * (or one rigid body and the world) are bound together.
 *
 * This is similar in concept to parenting one node to another,
 * but can be used in specific situations for a more appropriate effect
 * Ex. for implementing sticky projectiles, etc.
 */
class PhysicsFixedConstraint : public PhysicsGenericConstraint
{
    friend class PhysicsController;

  protected:
    /**
     * @see PhysicsGenericConstraint
     */
    PhysicsFixedConstraint(PhysicsRigidBody* a, PhysicsRigidBody* b);

    /**
     * Destructor.
     */
    ~PhysicsFixedConstraint();

    // Note: We make these functions protected to prevent usage
    // (these are public in the base class, PhysicsGenericConstraint).

    /**
     * Protected to prevent usage.
     */
    inline void setAngularLowerLimit(const Vector3& limit)
    {
        PhysicsGenericConstraint::setAngularLowerLimit(limit);
    }

    /**
     * Protected to prevent usage.
     */
    inline void setAngularUpperLimit(const Vector3& limit)
    {
        PhysicsGenericConstraint::setAngularUpperLimit(limit);
    }

    /**
     * Protected to prevent usage.
     */
    inline void setLinearLowerLimit(const Vector3& limit)
    {
        PhysicsGenericConstraint::setLinearLowerLimit(limit);
    }

    /**
     * Protected to prevent usage.
     */
    inline void setLinearUpperLimit(const Vector3& limit)
    {
        PhysicsGenericConstraint::setLinearUpperLimit(limit);
    }
};

} // namespace tractor
