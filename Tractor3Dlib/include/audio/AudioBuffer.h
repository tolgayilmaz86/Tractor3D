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

#include <AL/al.h>
#include <vorbis/vorbisfile.h>

#include "framework/Stream.h"
#include "utils/Ref.h"

namespace tractor
{

class AudioSource;

// State information for streaming a wav file
struct AudioStreamStateWav
{
    long dataStart;
    unsigned int dataSize;
    ALuint format;
    ALuint frequency;
};

// State information for streaming an ogg file
struct AudioStreamStateOgg
{
    long dataStart;
    unsigned int dataSize;
    ALuint format;
    ALuint frequency;
    OggVorbis_File oggFile;
};

/**
 * Defines the actual audio buffer data.
 *
 * Currently only supports supported formats: .ogg, .wav, .au and .raw files.
 */
class AudioBuffer : public Ref
{
    friend class AudioSource;

  public:
    /**
     * Constructor.
     */
    AudioBuffer(const std::string& path, ALuint* buffers, bool streamed);

    /**
     * Destructor.
     */
    ~AudioBuffer();

  private:
    /**
     * Hidden copy assignment operator.
     */
    AudioBuffer& operator=(const AudioBuffer&) = delete;

    /**
     * Streams data from the audio buffer into the specified OpenAL buffer.
     *
     * @param buffer The OpenAL buffer to stream data into.
     * @param looped Set to true if the audio should loop when it reaches the end.
     *
     * @return True if there is more data to stream, false if the end of the audio has been reached.
     */
    bool streamData(ALuint buffer, bool looped);

    /**
     * Creates an audio buffer from a file.
     *
     * @param path The path to the audio buffer on the filesystem.
     *
     * @return The buffer from a file.
     */
    static AudioBuffer* create(const std::string& path, bool streamed);

  private:
    static constexpr int STREAMING_BUFFER_QUEUE_SIZE = 3;

    int _buffersNeededCount;
    bool _streamed;
    ALuint _alBufferQueue[STREAMING_BUFFER_QUEUE_SIZE];
    std::string _filePath;
    std::unique_ptr<Stream> _fileStream;

    std::unique_ptr<AudioStreamStateWav> _streamStateWav;
    std::unique_ptr<AudioStreamStateOgg> _streamStateOgg;
};

} // namespace tractor
