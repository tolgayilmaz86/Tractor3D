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

#include <framework/FileStream.h>

#include <filesystem>
#include <fstream>

namespace tractor
{

//----------------------------------------------------------------------------
FileStream::FileStream(std::unique_ptr<std::fstream> stream) : _stream(std::move(stream)) {}

//----------------------------------------------------------------------------
FileStream::~FileStream()
{
    if (_stream && _stream->is_open()) close();
}

//----------------------------------------------------------------------------
std::unique_ptr<FileStream> FileStream::create(const std::string& filePath, const std::string& mode)
{
    namespace fs = std::filesystem;

    fs::path path(filePath);

    // Convert C-style mode to ios::openmode
    std::ios::openmode iosMode = std::ios::binary;

    if (mode.find('r') != std::string::npos)
    {
        iosMode |= std::ios::in;
    }
    if (mode.find('w') != std::string::npos)
    {
        iosMode |= std::ios::out | std::ios::trunc;
    }
    if (mode.find('a') != std::string::npos)
    {
        iosMode |= std::ios::out | std::ios::app;
    }
    if (mode.find('+') != std::string::npos)
    {
        iosMode |= std::ios::in | std::ios::out;
    }

    auto fstream = std::make_unique<std::fstream>(path, iosMode);

    if (!fstream->is_open()) return nullptr;

    auto stream = std::unique_ptr<FileStream>(new FileStream(std::move(fstream)));

    // Set read/write flags based on the mode
    stream->_canRead = (iosMode & std::ios::in) != 0;
    stream->_canWrite = (iosMode & std::ios::out) != 0;

    return stream;
}

//----------------------------------------------------------------------------
bool FileStream::canRead() { return _stream && _stream->is_open() && _canRead; }

//----------------------------------------------------------------------------
bool FileStream::canWrite() { return _stream && _stream->is_open() && _canWrite; }

//----------------------------------------------------------------------------
bool FileStream::canSeek() { return _stream && _stream->is_open(); }

//----------------------------------------------------------------------------
void FileStream::close()
{
    if (_stream && _stream->is_open()) _stream->close();
}

//----------------------------------------------------------------------------
size_t FileStream::read(void* ptr, size_t size, size_t count)
{
    if (!_stream || !canRead()) return 0;

    _stream->read(static_cast<char*>(ptr), size * count);

    std::streamsize bytesRead = _stream->gcount();
    return static_cast<size_t>(bytesRead / size);
}

//----------------------------------------------------------------------------
char* FileStream::readLine(char* str, int num)
{
    if (!_stream || !canRead()) return nullptr;

    _stream->getline(str, num);
    return (_stream->gcount() > 0) ? str : nullptr;
}

//----------------------------------------------------------------------------
size_t FileStream::write(const void* ptr, size_t size, size_t count)
{
    if (!_stream || !canWrite()) return 0;

    auto before = _stream->tellp();
    _stream->write(static_cast<const char*>(ptr), size * count);
    auto after = _stream->tellp();

    return (after - before) / size;
}

//----------------------------------------------------------------------------
bool FileStream::eof()
{
    if (!_stream) return true;
    return _stream->eof();
}

//----------------------------------------------------------------------------
size_t FileStream::length()
{
    if (!canSeek()) return 0;

    auto current = _stream->tellg();
    _stream->seekg(0, std::ios::end);
    auto length = _stream->tellg();
    _stream->seekg(current);

    return static_cast<size_t>(length);
}

//----------------------------------------------------------------------------
long int FileStream::position()
{
    if (!_stream) return -1;
    return static_cast<long int>(_stream->tellg());
}

//----------------------------------------------------------------------------
bool FileStream::seek(long int offset, int origin)
{
    if (!canSeek()) return false;

    std::ios::seekdir dir;
    switch (origin)
    {
        case SEEK_SET:
            dir = std::ios::beg;
            break;
        case SEEK_CUR:
            dir = std::ios::cur;
            break;
        case SEEK_END:
            dir = std::ios::end;
            break;
        default:
            return false;
    }

    // Clear error flags FIRST (including EOF)
    _stream->clear();
    _stream->seekg(offset, dir);

    // Check if seek failed
    if (_stream->fail())
    {
        _stream->clear(); // Clear the fail bit for future operations
        return false;
    }

    // Also update write position if stream is open for writing
    if (_canWrite)
    {
        _stream->seekp(offset, dir);
    }

    return true;
}

//----------------------------------------------------------------------------
bool FileStream::rewind()
{
    if (!canSeek()) return false;

    _stream->clear(); // Clear any error flags
    _stream->seekg(0, std::ios::beg);

    return _stream->good();
}
} // namespace tractor