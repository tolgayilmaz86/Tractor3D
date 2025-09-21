#include "physics/PhysicsRigidBody.h"
#include "pch.h"

namespace tractor
{

  inline float PhysicsRigidBody::getMass() const
  {
    return _mass;
  }

  inline float PhysicsRigidBody::getFriction() const
  {
    assert(_body);
    return _body->getFriction();
  }

  inline void PhysicsRigidBody::setFriction(float friction)
  {
    assert(_body);
    _body->setFriction(friction);
  }

  inline float PhysicsRigidBody::getRestitution() const
  {
    assert(_body);
    return _body->getRestitution();
  }

  inline void PhysicsRigidBody::setRestitution(float restitution)
  {
    assert(_body);
    _body->setRestitution(restitution);
  }

  inline float PhysicsRigidBody::getLinearDamping() const
  {
    assert(_body);
    return _body->getLinearDamping();
  }

  inline float PhysicsRigidBody::getAngularDamping() const
  {
    assert(_body);
    return _body->getAngularDamping();
  }

  inline void PhysicsRigidBody::setDamping(float linearDamping, float angularDamping)
  {
    assert(_body);
    _body->setDamping(linearDamping, angularDamping);
  }

  inline Vector3 PhysicsRigidBody::getLinearVelocity() const
  {
    assert(_body);
    const btVector3& v = _body->getLinearVelocity();
    return Vector3(v.x(), v.y(), v.z());
  }

  inline void PhysicsRigidBody::setLinearVelocity(const Vector3& velocity)
  {
    assert(_body);
    _body->setLinearVelocity(BV(velocity));
  }

  inline void PhysicsRigidBody::setLinearVelocity(float x, float y, float z)
  {
    assert(_body);
    _body->setLinearVelocity(btVector3(x, y, z));
  }

  inline Vector3 PhysicsRigidBody::getAngularVelocity() const
  {
    assert(_body);
    const btVector3& v = _body->getAngularVelocity();
    return Vector3(v.x(), v.y(), v.z());
  }

  inline void PhysicsRigidBody::setAngularVelocity(const Vector3& velocity)
  {
    assert(_body);
    _body->setAngularVelocity(BV(velocity));
  }

  inline void PhysicsRigidBody::setAngularVelocity(float x, float y, float z)
  {
    assert(_body);
    _body->setAngularVelocity(btVector3(x, y, z));
  }

  inline Vector3 PhysicsRigidBody::getAnisotropicFriction() const
  {
    assert(_body);
    const btVector3& af = _body->getAnisotropicFriction();
    return Vector3(af.x(), af.y(), af.z());
  }

  inline void PhysicsRigidBody::setAnisotropicFriction(const Vector3& friction)
  {
    assert(_body);
    _body->setAnisotropicFriction(BV(friction));
  }

  inline void PhysicsRigidBody::setAnisotropicFriction(float x, float y, float z)
  {
    assert(_body);
    _body->setAnisotropicFriction(btVector3(x, y, z));
  }

  inline Vector3 PhysicsRigidBody::getGravity() const
  {
    assert(_body);
    const btVector3& g = _body->getGravity();
    return Vector3(g.x(), g.y(), g.z());
  }

  inline void PhysicsRigidBody::setGravity(const Vector3& gravity)
  {
    assert(_body);
    _body->setGravity(BV(gravity));
  }

  inline void PhysicsRigidBody::setGravity(float x, float y, float z)
  {
    assert(_body);
    _body->setGravity(btVector3(x, y, z));
  }

  inline Vector3 PhysicsRigidBody::getAngularFactor() const
  {
    assert(_body);
    const btVector3& f = _body->getAngularFactor();
    return Vector3(f.x(), f.y(), f.z());
  }

  inline void PhysicsRigidBody::setAngularFactor(const Vector3& angularFactor)
  {
    assert(_body);
    _body->setAngularFactor(BV(angularFactor));
  }

  inline void PhysicsRigidBody::setAngularFactor(float x, float y, float z)
  {
    assert(_body);
    _body->setAngularFactor(btVector3(x, y, z));
  }

  inline Vector3 PhysicsRigidBody::getLinearFactor() const
  {
    assert(_body);
    const btVector3& f = _body->getLinearFactor();
    return Vector3(f.x(), f.y(), f.z());
  }

  inline void PhysicsRigidBody::setLinearFactor(const Vector3& angularFactor)
  {
    assert(_body);
    _body->setLinearFactor(BV(angularFactor));
  }

  inline void PhysicsRigidBody::setLinearFactor(float x, float y, float z)
  {
    assert(_body);
    _body->setLinearFactor(btVector3(x, y, z));
  }

  inline bool PhysicsRigidBody::isStatic() const
  {
    assert(_body);
    return _body->isStaticObject();
  }

}
