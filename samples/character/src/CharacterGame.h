#pragma once

#include "tractor.h"

/**
 * This is a mesh demo game for rendering Mesh.
 */
class CharacterGame : public tractor::Game,
                      public tractor::AnimationClip::Listener,
                      public tractor::PhysicsCollisionObject::CollisionListener
{
  public:
    /**
     * Constructor.
     */
    CharacterGame() = default;

    /**
     * @see Game::keyEvent
     */
    void keyEvent(tractor::Keyboard::KeyEvent evt, int key);

    /**
     * @see Game::touchEvent
     */
    void touchEvent(tractor::Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    /**
     * @see Game::mouseEvent
     */
    bool mouseEvent(tractor::Mouse::MouseEvent evt, int x, int y, int wheelDelta);

    /**
     * @see Game::gamepadEvent
     */
    void gamepadEvent(tractor::Gamepad::GamepadEvent evt, tractor::Gamepad* gamepad);

    /**
     * @see AnimationClip::Listener::animationEvent
     */
    void animationEvent(tractor::AnimationClip* clip,
                        tractor::AnimationClip::Listener::EventType type);

    /**
     * @see PhysicsCollisionObject::CollisionListener::collisionEvent
     */
    void collisionEvent(tractor::PhysicsCollisionObject::CollisionListener::EventType type,
                        const tractor::PhysicsCollisionObject::CollisionPair& collisionPair,
                        const tractor::Vector3& contactPointA = tractor::Vector3::zero(),
                        const tractor::Vector3& contactPointB = tractor::Vector3::zero());

  protected:
    /**
     * @see Game::initialize
     */
    void initialize();

    /**
     * @see Game::finalize
     */
    void finalize();

    /**
     * @see Game::update
     */
    void update(float elapsedTime);

    /**
     * @see Game::render
     */
    void render(float elapsedTime);

  private:
    bool initializeScene(tractor::Node* node);
    void initializeMaterial(tractor::Scene* scene, tractor::Node* node, tractor::Material* material);
    void initializeCharacter();
    void drawSplash(void* param);
    bool drawScene(tractor::Node* node, bool transparent);
    void play(const char* id, bool repeat, float speed = 1.0f);
    void jump();
    void kick();
    void adjustCamera(float elapsedTime);
    bool isOnFloor() const;
    void clone();
    void grabBall();
    void releaseBall();

  private:
    int _rotateX{ 0 };
    unsigned int _keyFlags{ 0 };
    bool _physicsDebug{ false };
    bool _wireframe{ false };
    bool _hasBall{ false };
    bool _applyKick{ false };
    bool _kicking{ false };
    bool _button1Pressed{ true };
    bool _button2Pressed{ true };
    float _floorLevel{ 0.0f };
    float _kickDelay{ 0.0f };

    tractor::Font* _font{ nullptr };
    tractor::Scene* _scene{ nullptr };
    tractor::PhysicsCharacter* _character{ nullptr };
    tractor::Node* _characterNode{ nullptr };
    tractor::Node* _characterMeshNode{ nullptr };
    tractor::Node* _characterShadowNode{ nullptr };
    tractor::Node* _basketballNode{ nullptr };
    tractor::Animation* _animation{ nullptr };
    tractor::AnimationClip* _currentClip{ nullptr };
    tractor::AnimationClip* _jumpClip{ nullptr };
    tractor::AnimationClip* _kickClip{ nullptr };
    tractor::Gamepad* _gamepad{ nullptr };
    tractor::MaterialParameter* _materialParameterAlpha{ nullptr };
    tractor::Vector2 _currentDirection{ tractor::Vector2::zero() };
    tractor::Vector3 _oldBallPosition{ tractor::Vector3::zero() };
};
