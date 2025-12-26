#pragma once

#include "tractor.h"

using namespace tractor;

/**
 * Main game class.
 */
class RacerGame : public Game, Control::Listener
{
  public:
    /**
     * Constructor.
     */
    RacerGame() = default;

    /**
     * @see Game::keyEvent
     */
    void keyEvent(Keyboard::KeyEvent evt, int key);

    /**
     * @see Game::touchEvent
     */
    void touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex);

    /**
     * @see Game::mouseEvent
     */
    bool mouseEvent(Mouse::MouseEvent evt, int x, int y, int wheelDelta);

    /**
     * @see Game::gamepadEvent
     */
    void gamepadEvent(Gamepad::GamepadEvent evt, Gamepad* gamepad);

    /**
     * @see Game::menuEvent
     */
    void menuEvent();

    /**
     * @see Control::controlEvent
     */
    void controlEvent(Control* control, EventType evt);

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
    /**
     * Initializes the scene.
     */
    bool initializeScene(Node* node);

    /**
     * Visits the scene to build render queues for a single frame.
     */
    bool buildRenderQueues(Node* node);

    /**
     * Draws the scene.
     */
    void drawScene();

    /**
     * Draws the splash screen.
     */
    void drawSplash(void* param);

    /**
     * Reset vehicle to its initial state.
     */
    void resetToStart();

    /**
     * Upright vehicle at its current location.
     */
    void resetInPlace();

    /**
     * Generic helper function for resets.
     *
     * @param pos desired position.
     * @param rot desired rotation.
     */
    void reset(const Vector3& pos, const Quaternion& rot);

    /**
     * Indicates that the vehicle may be over-turned.
     */
    bool isUpset() const;

    ScenePtr _scene;
    FontPtr _font{ nullptr };
    FormPtr _menu{ nullptr };
    FormPtr _overlay{ nullptr };
    std::vector<Node*> _renderQueues[2];
    unsigned int _keyFlags{ 0 };
    unsigned int _mouseFlags{ 0 };
    float _steering{ 0.0f };
    Gamepad* _gamepad{ nullptr };
    Gamepad* _physicalGamepad{ nullptr };
    Gamepad* _virtualGamepad{ nullptr };
    AnimationClip* _virtualGamepadClip{ nullptr };
    PhysicsVehicle* _carVehicle{ nullptr };
    float _upsetTimer{ 0.0f };

    // Music and Sounds
    AudioSourcePtr _backgroundMusic;
    AudioSourcePtr _engineSound;
    AudioSourcePtr _brakingSound;
};
