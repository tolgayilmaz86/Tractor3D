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
#include "pch.h"

#include <audio/AudioBuffer.h>
#include <audio/AudioController.h>
#include <audio/AudioListener.h>
#include <audio/AudioSource.h>

namespace tractor
{

AudioController::AudioController()
    : _alcDevice(nullptr), _alcContext(nullptr), _pausingSource(nullptr), _streamingThreadActive(true)
{
}

AudioController::~AudioController() {}

void AudioController::initialize()
{
    _alcDevice = alcOpenDevice(nullptr);
    if (!_alcDevice)
    {
        GP_ERROR("Unable to open OpenAL device.\n");
        return;
    }

    _alcContext = alcCreateContext(_alcDevice, nullptr);
    ALCenum alcErr = alcGetError(_alcDevice);
    if (!_alcContext || alcErr != ALC_NO_ERROR)
    {
        alcCloseDevice(_alcDevice);
        GP_ERROR("Unable to create OpenAL context. Error: %d\n", alcErr);
        return;
    }

    alcMakeContextCurrent(_alcContext);
    alcErr = alcGetError(_alcDevice);
    if (alcErr != ALC_NO_ERROR)
    {
        GP_ERROR("Unable to make OpenAL context current. Error: %d\n", alcErr);
    }
    _streamingMutex.reset(new std::mutex());
}

void AudioController::finalize()
{
    assert(_streamingSources.empty());
    if (_streamingThread)
    {
        _streamingThreadActive = false;
        if (_streamingThread->joinable())
        {
            _streamingThread->join(); // Join the thread before destruction
        }
        _streamingThread.reset(nullptr);
    }

    alcMakeContextCurrent(nullptr);
    if (_alcContext)
    {
        alcDestroyContext(_alcContext);
        _alcContext = nullptr;
    }
    if (_alcDevice)
    {
        alcCloseDevice(_alcDevice);
        _alcDevice = nullptr;
    }
}

void AudioController::pause()
{
    // For each source that is playing, pause it.
    for (auto& source : _playingSources)
    {
        assert(source);
        _pausingSource = source;

        source->pause();
        _pausingSource = nullptr;
    }
#ifdef ALC_SOFT_pause_device
    alcDevicePauseSOFT(_alcDevice);
#endif
}

void AudioController::resume()
{
    alcMakeContextCurrent(_alcContext);
#ifdef ALC_SOFT_pause_device
    alcDeviceResumeSOFT(_alcDevice);
#endif

    auto itr = _playingSources.begin();

    // For each source that is playing, resume it.
    for (auto& source : _playingSources)
    {
        assert(source);
        source->resume();
    }
}

void AudioController::update(float elapsedTime)
{
    AudioListener* listener = AudioListener::getInstance();
    if (listener)
    {
        AL_CHECK(alListenerf(AL_GAIN, listener->getGain()));
        AL_CHECK(alListenerfv(AL_ORIENTATION, (ALfloat*)listener->getOrientation()));
        AL_CHECK(alListenerfv(AL_VELOCITY, (ALfloat*)&listener->getVelocity()));
        AL_CHECK(alListenerfv(AL_POSITION, (ALfloat*)&listener->getPosition()));
    }
}

void AudioController::addPlayingSource(AudioSource* source)
{
    if (_playingSources.find(source) == _playingSources.end())
    {
        _playingSources.insert(source);

        if (source->isStreamed())
        {
            assert(_streamingSources.find(source) == _streamingSources.end());
            bool startThread = _streamingSources.empty() && _streamingThread.get() == nullptr;
            _streamingMutex->lock();
            _streamingSources.insert(source);
            _streamingMutex->unlock();

            if (startThread) _streamingThread.reset(new std::thread(&streamingThreadProc, this));
        }
    }
}

void AudioController::removePlayingSource(AudioSource* source)
{
    if (_pausingSource != source)
    {
        auto iter = std::find_if(_playingSources.begin(),
                                 _playingSources.end(),
                                 [source](const auto ptr) { return ptr == source; });

        if (iter != _playingSources.end())
        {
            _playingSources.erase(iter);

            if (source->isStreamed())
            {
                auto streaming_it = std::find_if(_streamingSources.begin(),
                                                 _streamingSources.end(),
                                                 [source](const auto& ptr) { return ptr == source; });

                assert(streaming_it != _streamingSources.end());
                _streamingMutex->lock();
                _streamingSources.erase(streaming_it);
                _streamingMutex->unlock();
            }
        }
    }
}

void AudioController::streamingThreadProc(void* arg)
{
    AudioController* controller = (AudioController*)arg;

    while (controller->_streamingThreadActive)
    {
        controller->_streamingMutex->lock();

        std::for_each(controller->_streamingSources.begin(),
                      controller->_streamingSources.end(),
                      std::mem_fn(&AudioSource::streamDataIfNeeded));

        controller->_streamingMutex->unlock();

        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    }
}

} // namespace tractor
