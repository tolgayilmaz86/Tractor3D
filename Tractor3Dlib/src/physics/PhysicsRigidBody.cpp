#include "framework/Base.h"
#include "physics/PhysicsRigidBody.h"
#include "physics/PhysicsController.h"
#include "framework/Game.h"
#include "ui/Image.h"
#include "graphics/MeshPart.h"
#include "scene/Node.h"
#include "graphics/Terrain.h"

namespace tractor
{

  PhysicsRigidBody::PhysicsRigidBody(Node* node, const PhysicsCollisionShape::Definition& shape, const Parameters& parameters, int group, int mask)
    : PhysicsCollisionObject(node, group, mask), _body(nullptr), _mass(parameters.mass), _constraints(nullptr), _inDestructor(false)
  {
    assert(Game::getInstance()->getPhysicsController());
    assert(_node);

    // Create our collision shape.
    Vector3 centerOfMassOffset;
    _collisionShape = Game::getInstance()->getPhysicsController()->createShape(node, shape, &centerOfMassOffset, parameters.mass != 0.0f);
    assert(_collisionShape && _collisionShape->getShape());

    // Create motion state object.
    _motionState = new PhysicsMotionState(node, this, (centerOfMassOffset.lengthSquared() > MATH_EPSILON) ? &centerOfMassOffset : nullptr);

    // If the mass is non-zero, then the object is dynamic so we calculate the local 
    // inertia. However, if the collision shape is a triangle mesh, we don't calculate 
    // inertia since Bullet doesn't currently support this.
    btVector3 localInertia(0.0, 0.0, 0.0);
    if (parameters.mass != 0.0)
      _collisionShape->getShape()->calculateLocalInertia(parameters.mass, localInertia);

    // Create the Bullet physics rigid body object.
    btRigidBody::btRigidBodyConstructionInfo rbInfo(parameters.mass, nullptr, _collisionShape->getShape(), localInertia);
    rbInfo.m_friction = parameters.friction;
    rbInfo.m_restitution = parameters.restitution;
    rbInfo.m_linearDamping = parameters.linearDamping;
    rbInfo.m_angularDamping = parameters.angularDamping;

    // Create + assign the new bullet rigid body object.
    _body = bullet_new<btRigidBody>(rbInfo);

    // Set motion state after rigid body assignment, since bullet will callback on the motion state interface to query
    // the initial transform and it will need to access to rigid body (_body).
    _body->setMotionState(_motionState);

    // Set other initially defined properties.
    setKinematic(parameters.kinematic);
    setAnisotropicFriction(parameters.anisotropicFriction);
    setAngularFactor(parameters.angularFactor);
    setLinearFactor(parameters.linearFactor);

    // Add ourself to the physics world.
    Game::getInstance()->getPhysicsController()->addCollisionObject(this);

    if (_collisionShape->getType() == PhysicsCollisionShape::SHAPE_HEIGHTFIELD)
    {
      // Add a listener on the node's transform so we can track dirty changes to calculate
      // an inverse matrix for transforming heightfield points between world and local space.
      _node->addListener(this);
    }
  }

  PhysicsRigidBody::~PhysicsRigidBody()
  {
    assert(Game::getInstance()->getPhysicsController());
    assert(_collisionShape);
    assert(_node);

    // Clean up all constraints linked to this rigid body.
    _inDestructor = true;
    if (_constraints)
    {
      for (unsigned int i = 0; i < _constraints->size(); ++i)
      {
        SAFE_DELETE((*_constraints)[i]);
      }
      SAFE_DELETE(_constraints);
    }

    // Remove collision object from physics controller.
    Game::getInstance()->getPhysicsController()->removeCollisionObject(this, true);

    // Clean up the rigid body and its related objects.
    SAFE_DELETE(_body);

    // Unregister node listener (only registered for heihgtfield collision shape types).
    if (_collisionShape->getType() == PhysicsCollisionShape::SHAPE_HEIGHTFIELD)
    {
      _node->removeListener(this);
    }
  }

  PhysicsCollisionObject::Type PhysicsRigidBody::getType() const
  {
    return PhysicsCollisionObject::RIGID_BODY;
  }

  btCollisionObject* PhysicsRigidBody::getCollisionObject() const
  {
    return _body;
  }

  void PhysicsRigidBody::applyForce(const Vector3& force, const Vector3* relativePosition)
  {
    // If the force is significant enough, activate the rigid body 
    // to make sure that it isn't sleeping and apply the force.
    if (force.lengthSquared() > MATH_EPSILON)
    {
      assert(_body);
      _body->activate();
      if (relativePosition)
        _body->applyForce(BV(force), BV(*relativePosition));
      else
        _body->applyCentralForce(BV(force));
    }
  }

  void PhysicsRigidBody::applyImpulse(const Vector3& impulse, const Vector3* relativePosition)
  {
    // If the impulse is significant enough, activate the rigid body 
    // to make sure that it isn't sleeping and apply the impulse.
    if (impulse.lengthSquared() > MATH_EPSILON)
    {
      assert(_body);
      _body->activate();
      if (relativePosition)
      {
        _body->applyImpulse(BV(impulse), BV(*relativePosition));
      }
      else
        _body->applyCentralImpulse(BV(impulse));
    }
  }

  void PhysicsRigidBody::applyTorque(const Vector3& torque)
  {
    // If the torque is significant enough, activate the rigid body 
    // to make sure that it isn't sleeping and apply the torque.
    if (torque.lengthSquared() > MATH_EPSILON)
    {
      assert(_body);
      _body->activate();
      _body->applyTorque(BV(torque));
    }
  }

  void PhysicsRigidBody::applyTorqueImpulse(const Vector3& torque)
  {
    // If the torque impulse is significant enough, activate the rigid body 
    // to make sure that it isn't sleeping and apply the torque impulse.
    if (torque.lengthSquared() > MATH_EPSILON)
    {
      assert(_body);
      _body->activate();
      _body->applyTorqueImpulse(BV(torque));
    }
  }

  PhysicsRigidBody* PhysicsRigidBody::create(Node* node, Properties* properties, const char* nspace)
  {
    // Check if the properties is valid and has a valid namespace.
    if (!properties || !(strcmp(properties->getNamespace(), "collisionObject") == 0))
    {
      GP_ERROR("Failed to load rigid body from properties object: must be non-null object and have namespace equal to 'collisionObject'.");
      return nullptr;
    }

    // Check that the type is specified and correct.
    const char* type = properties->getString("type");
    if (!type)
    {
      GP_ERROR("Failed to load physics rigid body from properties object; required attribute 'type' is missing.");
      return nullptr;
    }
    if (strcmp(type, nspace) != 0)
    {
      GP_ERROR("Failed to load physics rigid body from properties object; attribute 'type' must be equal to '%s'.", nspace);
      return nullptr;
    }

    // Load the physics collision shape definition.
    PhysicsCollisionShape::Definition shape = PhysicsCollisionShape::Definition::create(node, properties);
    if (shape.isEmpty())
    {
      GP_ERROR("Failed to create collision shape during rigid body creation.");
      return nullptr;
    }

    // Set the rigid body parameters to their defaults.
    Parameters parameters;
    Vector3* gravity = nullptr;

    // Load the defined rigid body parameters.
    properties->rewind();
    const char* name;
    while ((name = properties->getNextProperty()) != nullptr)
    {
      if (strcmp(name, "mass") == 0)
      {
        parameters.mass = properties->getFloat();
      }
      else if (strcmp(name, "friction") == 0)
      {
        parameters.friction = properties->getFloat();
      }
      else if (strcmp(name, "restitution") == 0)
      {
        parameters.restitution = properties->getFloat();
      }
      else if (strcmp(name, "linearDamping") == 0)
      {
        parameters.linearDamping = properties->getFloat();
      }
      else if (strcmp(name, "angularDamping") == 0)
      {
        parameters.angularDamping = properties->getFloat();
      }
      else if (strcmp(name, "kinematic") == 0)
      {
        parameters.kinematic = properties->getBool();
      }
      else if (strcmp(name, "anisotropicFriction") == 0)
      {
        properties->getVector3(nullptr, &parameters.anisotropicFriction);
      }
      else if (strcmp(name, "gravity") == 0)
      {
        gravity = new Vector3();
        properties->getVector3(nullptr, gravity);
      }
      else if (strcmp(name, "angularFactor") == 0)
      {
        properties->getVector3(nullptr, &parameters.angularFactor);
      }
      else if (strcmp(name, "linearFactor") == 0)
      {
        properties->getVector3(nullptr, &parameters.linearFactor);
      }
      else
      {
        // Ignore this case (the attributes for the rigid body's collision shape would end up here).
      }
    }

    // Create the rigid body.
    PhysicsRigidBody* body = new PhysicsRigidBody(node, shape, parameters);

    if (gravity)
    {
      body->setGravity(*gravity);
      SAFE_DELETE(gravity);
    }

    return body;
  }

  void PhysicsRigidBody::setKinematic(bool kinematic)
  {
    assert(_body);

    if (kinematic)
    {
      _body->setCollisionFlags(_body->getCollisionFlags() | btCollisionObject::CF_KINEMATIC_OBJECT);
      _body->setActivationState(DISABLE_DEACTIVATION);
    }
    else
    {
      _body->setCollisionFlags(_body->getCollisionFlags() & ~btCollisionObject::CF_KINEMATIC_OBJECT);
      _body->setActivationState(ACTIVE_TAG);
    }
  }

  void PhysicsRigidBody::setEnabled(bool enable)
  {
    PhysicsCollisionObject::setEnabled(enable);
    if (enable)
      _body->setMotionState(_motionState);
  }

  float PhysicsRigidBody::getHeight(float x, float z) const
  {
    assert(_collisionShape);
    assert(_node);

    // If our node has a terrain, call getHeight() on it since we need to factor in local
    // scaling on the terrain into the height calculation.
    Terrain* terrain = dynamic_cast<Terrain*>(_node->getDrawable());
    if (terrain)
      return terrain->getHeight(x, z);

    // This function is only supported for heightfield rigid bodies.
    if (_collisionShape->getType() != PhysicsCollisionShape::SHAPE_HEIGHTFIELD)
    {
      GP_WARN("Attempting to get the height of a non-heightfield rigid body.");
      return 0.0f;
    }

    assert(_collisionShape->_shapeData.heightfieldData);

    // Ensure inverse matrix is updated so we can transform from world back into local heightfield coordinates for indexing
    if (_collisionShape->_shapeData.heightfieldData->inverseIsDirty)
    {
      _collisionShape->_shapeData.heightfieldData->inverseIsDirty = false;

      _node->getWorldMatrix().invert(&_collisionShape->_shapeData.heightfieldData->inverse);
    }

    // Calculate the correct x, z position relative to the heightfield data.
    float cols = _collisionShape->_shapeData.heightfieldData->heightfield->getColumnCount();
    float rows = _collisionShape->_shapeData.heightfieldData->heightfield->getRowCount();

    assert(cols > 0);
    assert(rows > 0);

    Vector3 v = _collisionShape->_shapeData.heightfieldData->inverse * Vector3(x, 0.0f, z);
    x = v.x + (cols - 1) * 0.5f;
    z = v.z + (rows - 1) * 0.5f;

    // Get the unscaled height value from the HeightField
    float height = _collisionShape->_shapeData.heightfieldData->heightfield->getHeight(x, z);

    // Apply scale back to height
    Vector3 worldScale;
    _node->getWorldMatrix().getScale(&worldScale);
    height *= worldScale.y;

    return height;
  }

  void PhysicsRigidBody::addConstraint(PhysicsConstraint* constraint)
  {
    assert(constraint);
    if (_constraints == nullptr)
      _constraints = new std::vector<PhysicsConstraint*>();

    _constraints->push_back(constraint);
  }

  void PhysicsRigidBody::removeConstraint(PhysicsConstraint* constraint)
  {
    // Ensure that the rigid body has constraints and that we are
    // not currently in the rigid body's destructor (in this case,
    // the constraints will be destroyed from there).
    if (_constraints && !_inDestructor)
    {
      for (unsigned int i = 0; i < _constraints->size(); ++i)
      {
        if ((*_constraints)[i] == constraint)
        {
          _constraints->erase(_constraints->begin() + i);
          break;
        }
      }
    }
  }

  bool PhysicsRigidBody::supportsConstraints()
  {
    return (getShapeType() != PhysicsCollisionShape::SHAPE_HEIGHTFIELD && getShapeType() != PhysicsCollisionShape::SHAPE_MESH);
  }

  void PhysicsRigidBody::transformChanged(Transform* transform, long cookie)
  {
    if (getShapeType() == PhysicsCollisionShape::SHAPE_HEIGHTFIELD)
    {
      assert(_collisionShape && _collisionShape->_shapeData.heightfieldData);

      // Dirty the heightfield's inverse matrix (used to compute height values from world-space coordinates)
      _collisionShape->_shapeData.heightfieldData->inverseIsDirty = true;

      // Update local scaling for the heightfield.
      Vector3 scale;
      _node->getWorldMatrix().getScale(&scale);

      // If the node has a terrain attached, factor in the terrain local scaling as well for the collision shape
      Terrain* terrain = dynamic_cast<Terrain*>(_node->getDrawable());
      if (terrain)
      {
        const Vector3& tScale = terrain->_localScale;
        scale.set(scale.x * tScale.x, scale.y * tScale.y, scale.z * tScale.z);
      }

      _collisionShape->_shape->setLocalScaling(BV(scale));

      // Update center of mass offset
      float minHeight = _collisionShape->_shapeData.heightfieldData->minHeight;
      float maxHeight = _collisionShape->_shapeData.heightfieldData->maxHeight;
      _motionState->setCenterOfMassOffset(Vector3(0, -(minHeight + (maxHeight - minHeight) * 0.5f) * scale.y, 0));
    }
  }

}
