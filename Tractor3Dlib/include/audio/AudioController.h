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

namespace tractor
{

class AudioListener;
class AudioSource;

/**
 * Defines a class for controlling game audio.
 */
class AudioController
{
    friend class Game;
    friend class AudioSource;

  public:
    /**
     * Constructor.
     */
    AudioController() = default;

    /**
     * Destructor.
     */
    virtual ~AudioController() = default;

  private:
    /**
     * Controller initialize.
     */
    void initialize();

    /**
     * Controller finalize.
     */
    void finalize();

    /**
     * Controller pause.
     */
    void pause();

    /**
     * Controller resume.
     */
    void resume();

    /**
     * Controller update.
     */
    void update(float elapsedTime);

    /**
     * Adds a playing audio source to the controller.
     */
    void addPlayingSource(AudioSource* source);

    /**
     * Removes a playing audio source from the controller.
     */
    void removePlayingSource(AudioSource* source);

    /**
     * Thread procedure for streaming audio sources.
     */
    void streamingThreadProc(std::stop_token stopToken);

  private:
    ALCdevice* _alcDevice{ nullptr };
    ALCcontext* _alcContext{ nullptr };
    std::set<AudioSource*> _playingSources;
    std::set<AudioSource*> _streamingSources;
    AudioSource* _pausingSource;

    bool _streamingThreadActive{ true };
    std::unique_ptr<std::jthread> _streamingThread;
    std::unique_ptr<std::mutex> _streamingMutex;
    std::unique_ptr<std::condition_variable> _streamingCV;

};

} // namespace tractor
