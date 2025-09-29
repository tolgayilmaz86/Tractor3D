#pragma once

#include "math/Vector3.h"
#include "math/Quaternion.h"

namespace tractor
{
class PhysicsRigidBody;
class Node;

/**
 * Defines the base class for physics constraints.
 */
class PhysicsConstraint
{
    friend class PhysicsController;
    friend class PhysicsRigidBody;

  public:
    /**
     * Gets the impulse needed to break the constraint.
     *
     * @return The impulse needed to break the constraint.
     */
    float getBreakingImpulse() const { return _constraint->getBreakingImpulseThreshold(); }

    /**
     * Sets the impulse needed to break the constraint
     * (if an impulse greater than or equal to the given
     * value is applied to the constraint, the constraint
     * will be broken).
     *
     * @param impulse The impulse needed to break the constraint.
     */
    void setBreakingImpulse(float impulse) { _constraint->setBreakingImpulseThreshold(impulse); }

    /**
     * Gets whether the constraint is enabled or not.
     *
     * @return Whether the constraint is enabled or not.
     */
    bool isEnabled() const { return _constraint->isEnabled(); }

    /**
     * Sets whether the constraint is enabled or not.
     *
     * @param enabled Whether the constraint is enabled or not.
     */
    void setEnabled(bool enabled) { _constraint->setEnabled(enabled); }

    /**
     * Calculates the midpoint between the given nodes' centers of mass.
     *
     * @param a The first node.
     * @param b The second node.
     */
    static Vector3 centerOfMassMidpoint(const Node* a, const Node* b);

    /**
     * Calculates the rotation offset to the given point in the given node's local space.
     *
     * @param node The node to calculate a rotation offset for.
     * @param point The point to calculate the rotation offset to.
     */
    static Quaternion getRotationOffset(const Node* node, const Vector3& point);

    /**
     * Calculates the translation offset to the given point in the given node's local space.
     *
     * @param node The node to calculate a translation offset for.
     * @param point The point to calculate the translation offset to.
     */
    static Vector3 getTranslationOffset(const Node* node, const Vector3& point);

  protected:
    /**
     * Constructor.
     */
    PhysicsConstraint(PhysicsRigidBody* a, PhysicsRigidBody* b);

    /**
     * Destructor.
     */
    virtual ~PhysicsConstraint();

    /**
     * Calculates the transform to be used as the offset (i.e. "frame in"
     * parameter in Bullet terms) to the given constraint origin.
     */
    static btTransform getTransformOffset(const Node* node, const Vector3& origin);

    /**
     * Calculates the center of mass in world space of the given node.
     */
    static Vector3 getWorldCenterOfMass(const Node* node);

    /**
     * Offsets the given vector by the given node's center of mass.
     */
    static Vector3 offsetByCenterOfMass(const Node* node, const Vector3& v);

    /**
     * Pointer to the one rigid body bound by this constraint.
     */
    PhysicsRigidBody* _a;

    /**
     * Pointer to the other rigid body bound by this constraint.
     */
    PhysicsRigidBody* _b;

    /**
     * Pointer to the Bullet constraint.
     */
    btTypedConstraint* _constraint;
};

} // namespace tractor
