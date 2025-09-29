#pragma once

#include "math/Quaternion.h"
#include "math/Vector3.h"
#include "physics/PhysicsConstraint.h"

namespace tractor
{
class PhysicsRigidBody;

/**
 * Defines a completely generic constraint between two
 * rigid bodies (or one rigid body and the world) where the
 * limits for all six degrees of freedom can be set individually.
 */
class PhysicsGenericConstraint : public PhysicsConstraint
{
    friend class PhysicsController;

  public:
    /**
     * Gets the rotation offset for the first rigid body in the constraint.
     *
     * @return The rotation offset.
     */
    inline const Quaternion& getRotationOffsetA() const
    {
        if (!_rotationOffsetA) _rotationOffsetA = new Quaternion();

        assert(_constraint);
        btQuaternion ro =
            static_cast<btGeneric6DofConstraint*>(_constraint)->getFrameOffsetA().getRotation();
        _rotationOffsetA->set(ro.x(), ro.y(), ro.z(), ro.w());
        return *_rotationOffsetA;
    }

    /**
     * Gets the rotation offset for the second rigid body in the constraint.
     *
     * @return The rotation offset.
     */

  inline const Quaternion& getRotationOffsetB() const
    {
        if (!_rotationOffsetB) _rotationOffsetB = new Quaternion();

        assert(_constraint);
        btQuaternion ro =
            static_cast<btGeneric6DofConstraint*>(_constraint)->getFrameOffsetB().getRotation();
        _rotationOffsetB->set(ro.x(), ro.y(), ro.z(), ro.w());
        return *_rotationOffsetB;
    }

    /**
     * Gets the translation offset for the first rigid body in the constraint.
     *
     * @return The translation offset.
     */
    inline const Vector3& getTranslationOffsetA() const
    {
        if (!_translationOffsetA) _translationOffsetA = new Vector3();

        assert(_constraint);
        btVector3 to =
            static_cast<btGeneric6DofConstraint*>(_constraint)->getFrameOffsetA().getOrigin();
        _translationOffsetA->set(to.x(), to.y(), to.z());
        return *_translationOffsetA;
    }

    /**
     * Gets the translation offset for the second rigid body in the constraint.
     *
     * @return The translation offset.
     */
    inline const Vector3& getTranslationOffsetB() const
    {
        if (!_translationOffsetB) _translationOffsetB = new Vector3();

        assert(_constraint);
        btVector3 to =
            static_cast<btGeneric6DofConstraint*>(_constraint)->getFrameOffsetB().getOrigin();
        _translationOffsetB->set(to.x(), to.y(), to.z());
        return *_translationOffsetB;
    }

    /**
     * Sets the lower angular limits (as Euler angle limits) along the constraint's local
     * X, Y, and Z axes using the values in the given vector.
     *
     * @param limits The lower angular limits (as Euler angle limits) along the local X, Y, and Z axes.
     */
    inline void setAngularLowerLimit(const Vector3& limits)
    {
        assert(_constraint);
        ((btGeneric6DofConstraint*)_constraint)->setAngularLowerLimit(BV(limits));
    }

    /**
     * Sets the upper angular limits (as Euler angle limits) along the constraint's local
     * X, Y, and Z axes using the values in the given vector.
     *
     * @param limits The upper angular limits (as Euler angle limits) along the local X, Y, and Z axes.
     */
    inline void setAngularUpperLimit(const Vector3& limits)
    {
        assert(_constraint);
        ((btGeneric6DofConstraint*)_constraint)->setAngularUpperLimit(BV(limits));
    }

    /**
     * Sets the lower linear limits along the constraint's local
     * X, Y, and Z axes using the values in the given vector.
     *
     * @param limits The lower linear limits along the local X, Y, and Z axes.
     */
    inline void setLinearLowerLimit(const Vector3& limits)
    {
        assert(_constraint);
        ((btGeneric6DofConstraint*)_constraint)->setLinearLowerLimit(BV(limits));
    }

    /**
     * Sets the upper linear limits along the constraint's local
     * X, Y, and Z axes using the values in the given vector.
     *
     * @param limits The upper linear limits along the local X, Y, and Z axes.
     */
    inline void setLinearUpperLimit(const Vector3& limits)
    {
        assert(_constraint);
        ((btGeneric6DofConstraint*)_constraint)->setLinearUpperLimit(BV(limits));
    }

    /**
     * Sets the rotation offset for the first rigid body in the constraint.
     *
     * @param rotationOffset The rotation offset.
     */
    inline void setRotationOffsetA(const Quaternion& rotationOffset)
    {
        assert(_constraint);
        static_cast<btGeneric6DofConstraint*>(_constraint)
            ->getFrameOffsetA()
            .setRotation(BQ(rotationOffset));
    }

    /**
     * Sets the rotation offset for the second rigid body in the constraint.
     *
     * @param rotationOffset The rotation offset.
     */
    inline void setRotationOffsetB(const Quaternion& rotationOffset)
    {
        assert(_constraint);
        static_cast<btGeneric6DofConstraint*>(_constraint)
            ->getFrameOffsetB()
            .setRotation(BQ(rotationOffset));
    }

    /**
     * Sets the translation offset for the first rigid body in the constraint.
     *
     * @param translationOffset The translation offset.
     */
    inline void setTranslationOffsetA(const Vector3& translationOffset)
    {
        assert(_constraint);
        static_cast<btGeneric6DofConstraint*>(_constraint)
            ->getFrameOffsetA()
            .setOrigin(BV(translationOffset));
    }

    /**
     * Sets the translation offset for the second rigid body in the constraint.
     *
     * @param translationOffset The translation offset.
     */
    inline void setTranslationOffsetB(const Vector3& translationOffset)
    {
        assert(_constraint);
        static_cast<btGeneric6DofConstraint*>(_constraint)
            ->getFrameOffsetB()
            .setOrigin(BV(translationOffset));
    }

    inline const Quaternion& PgetRotationOffsetA() const
    {
        if (!_rotationOffsetA) _rotationOffsetA = new Quaternion();

        assert(_constraint);
        btQuaternion ro =
            static_cast<btGeneric6DofConstraint*>(_constraint)->getFrameOffsetA().getRotation();
        _rotationOffsetA->set(ro.x(), ro.y(), ro.z(), ro.w());
        return *_rotationOffsetA;
    }

    inline const Quaternion& PgetRotationOffsetB() const
    {
        if (!_rotationOffsetB) _rotationOffsetB = new Quaternion();

        assert(_constraint);
        btQuaternion ro =
            static_cast<btGeneric6DofConstraint*>(_constraint)->getFrameOffsetB().getRotation();
        _rotationOffsetB->set(ro.x(), ro.y(), ro.z(), ro.w());
        return *_rotationOffsetB;
    }

    inline const Vector3& PgetTranslationOffsetA() const
    {
        if (!_translationOffsetA) _translationOffsetA = new Vector3();

        assert(_constraint);
        btVector3 to =
            static_cast<btGeneric6DofConstraint*>(_constraint)->getFrameOffsetA().getOrigin();
        _translationOffsetA->set(to.x(), to.y(), to.z());
        return *_translationOffsetA;
    }

    inline const Vector3& PgetTranslationOffsetB() const
    {
        if (!_translationOffsetB) _translationOffsetB = new Vector3();

        assert(_constraint);
        btVector3 to =
            static_cast<btGeneric6DofConstraint*>(_constraint)->getFrameOffsetB().getOrigin();
        _translationOffsetB->set(to.x(), to.y(), to.z());
        return *_translationOffsetB;
    }

    inline void PsetAngularLowerLimit(const Vector3& limits)
    {
        assert(_constraint);
        ((btGeneric6DofConstraint*)_constraint)->setAngularLowerLimit(BV(limits));
    }

    inline void PsetAngularUpperLimit(const Vector3& limits)
    {
        assert(_constraint);
        ((btGeneric6DofConstraint*)_constraint)->setAngularUpperLimit(BV(limits));
    }

    inline void PsetLinearLowerLimit(const Vector3& limits)
    {
        assert(_constraint);
        ((btGeneric6DofConstraint*)_constraint)->setLinearLowerLimit(BV(limits));
    }

    inline void PsetLinearUpperLimit(const Vector3& limits)
    {
        assert(_constraint);
        ((btGeneric6DofConstraint*)_constraint)->setLinearUpperLimit(BV(limits));
    }

    inline void PsetRotationOffsetA(const Quaternion& rotationOffset)
    {
        assert(_constraint);
        static_cast<btGeneric6DofConstraint*>(_constraint)
            ->getFrameOffsetA()
            .setRotation(BQ(rotationOffset));
    }

    inline void PsetRotationOffsetB(const Quaternion& rotationOffset)
    {
        assert(_constraint);
        static_cast<btGeneric6DofConstraint*>(_constraint)
            ->getFrameOffsetB()
            .setRotation(BQ(rotationOffset));
    }

    inline void PsetTranslationOffsetA(const Vector3& translationOffset)
    {
        assert(_constraint);
        static_cast<btGeneric6DofConstraint*>(_constraint)
            ->getFrameOffsetA()
            .setOrigin(BV(translationOffset));
    }

    inline void PsetTranslationOffsetB(const Vector3& translationOffset)
    {
        assert(_constraint);
        static_cast<btGeneric6DofConstraint*>(_constraint)
            ->getFrameOffsetB()
            .setOrigin(BV(translationOffset));
    }

  protected:
    /**
     * Constructor.
     *
     * Note: This should only used by subclasses that do not want
     * the _constraint member variable to be initialized.
     */
    PhysicsGenericConstraint();

    /**
     * Creates a generic constraint so that the rigid body (or bodies) is
     * (are) constrained to its (their) current world position(s).
     *
     * @param a The first (possibly only) rigid body to constrain. If this is the only rigid
     *      body specified the constraint applies between it and the global physics world object.
     * @param b The second rigid body to constrain (optional).
     */
    PhysicsGenericConstraint(PhysicsRigidBody* a, PhysicsRigidBody* b);

    /**
     * Creates a generic constraint.
     *
     * @param a The first (possibly only) rigid body to constrain. If this is the only rigid
     *      body specified the constraint applies between it and the global physics world object.
     * @param rotationOffsetA The rotation offset for the first rigid body
     *      (in its local space) with respect to the constraint joint.
     * @param translationOffsetA The translation offset for the first rigid body
     *      (in its local space) with respect to the constraint joint.
     * @param b The second rigid body to constrain (optional).
     * @param rotationOffsetB The rotation offset for the second rigid body
     *      (in its local space) with respect to the constraint joint (optional).
     * @param translationOffsetB The translation offset for the second rigid body
     *      (in its local space) with respect to the constraint joint (optional).
     */
    PhysicsGenericConstraint(PhysicsRigidBody* a,
                             const Quaternion& rotationOffsetA,
                             const Vector3& translationOffsetA,
                             PhysicsRigidBody* b,
                             const Quaternion& rotationOffsetB,
                             const Vector3& translationOffsetB);

    /**
     * Destructor.
     */
    virtual ~PhysicsGenericConstraint();

  private:
    mutable Quaternion* _rotationOffsetA;
    mutable Quaternion* _rotationOffsetB;
    mutable Vector3* _translationOffsetA;
    mutable Vector3* _translationOffsetB;
};

} // namespace tractor
