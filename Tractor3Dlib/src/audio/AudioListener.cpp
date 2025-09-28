#include "pch.h"

#include <audio/AudioListener.h>
#include <framework/Game.h>
#include <scene/Node.h>

namespace tractor
{

AudioListener::~AudioListener()
{
    // Call setCamera() to release camera and cause transform listener
    // to be removed.
    setCamera(nullptr);
}

AudioListener* AudioListener::getInstance() { return Game::getInstance()->getAudioListener(); }

void AudioListener::setOrientation(const Vector3& forward, const Vector3& up)
{
    _orientation[0].x = forward.x;
    _orientation[0].y = forward.y;
    _orientation[0].z = forward.z;

    _orientation[1].x = up.x;
    _orientation[1].y = up.y;
    _orientation[1].z = up.z;
}

void AudioListener::setOrientation(float forwardX,
                                   float forwardY,
                                   float forwardZ,
                                   float upX,
                                   float upY,
                                   float upZ)
{
    _orientation[0].set(forwardX, forwardY, forwardZ);
    _orientation[1].set(upX, upY, upZ);
}

void AudioListener::setCamera(Camera* camera)
{
    if (_camera == camera) return;

    // Disconnect our current camera.
    if (_camera)
    {
        _camera->removeListener(this);
        SAFE_RELEASE(_camera);
    }

    // Connect the new camera.
    _camera = camera;
    if (_camera)
    {
        _camera->addRef();
        _camera->addListener(this);
    }
}

void AudioListener::cameraChanged(Camera* camera)
{
    if (_camera != camera) setCamera(camera);

    if (_camera)
    {
        Node* node = camera->getNode();
        if (node)
        {
            setPosition(node->getTranslationWorld());
            Vector3 up = node->getWorldMatrix().getUpVector();
            setOrientation(node->getForwardVectorWorld(), up);
        }
        else
        {
            setPosition(Vector3::zero());
            setOrientation(Vector3::unitY(), -Vector3::unitZ());
        }
    }
}

} // namespace tractor
