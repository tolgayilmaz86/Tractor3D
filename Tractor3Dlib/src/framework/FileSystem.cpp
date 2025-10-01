#include "pch.h"

#include <framework/FileSystem.h>
#include <framework/Platform.h>
#include <framework/Stream.h>
#include <scene/Properties.h>

#include <filesystem>
#include <fstream>

namespace tractor
{

/** @script{ignore} */
static std::string g_resourcePath("./");
static std::string __assetPath("");
static std::map<std::string, std::string> __aliases;

/**
 * Gets the fully resolved path.
 * If the path is relative then it will be prefixed with the resource path.
 * Aliases will be converted to a relative path.
 *
 * @param path The path to resolve.
 * @param fullPath The full resolved path. (out param)
 */
static void getFullPath(const std::string& path, std::string& fullPath)
{
    if (FileSystem::isAbsolutePath(path))
    {
        fullPath.assign(path);
    }
    else
    {
        fullPath.assign(g_resourcePath);
        fullPath += FileSystem::resolvePath(path);
    }
}

/**
 *
 * @script{ignore}
 */
class FileStream : public Stream
{
  public:
    friend class FileSystem;

    ~FileStream();
    virtual bool canRead();
    virtual bool canWrite();
    virtual bool canSeek();
    virtual void close();
    virtual size_t read(void* ptr, size_t size, size_t count);
    virtual char* readLine(char* str, int num);
    virtual size_t write(const void* ptr, size_t size, size_t count);
    virtual bool eof();
    virtual size_t length();
    virtual long int position();
    virtual bool seek(long int offset, int origin);
    virtual bool rewind();

    static std::unique_ptr<FileStream> create(const std::string& filePath, const std::string& mode);

  private:
    FileStream(std::unique_ptr<std::fstream> stream);

  private:
    std::unique_ptr<std::fstream> _stream;
    bool _canRead{ false };
    bool _canWrite{ false };
};

void FileSystem::setResourcePath(const std::string& path)
{
    g_resourcePath = path.empty() ? "" : path;
}

std::string FileSystem::getResourcePath() { return g_resourcePath; }

void FileSystem::loadResourceAliases(const std::string& aliasFilePath)
{
    auto properties = std::unique_ptr<Properties>(Properties::create(aliasFilePath));
    if (properties)
    {
        Properties* aliases;
        while ((aliases = properties->getNextNamespace()) != nullptr)
        {
            loadResourceAliases(aliases);
        }
    }
}

void FileSystem::loadResourceAliases(Properties* properties)
{
    assert(properties);

    while (auto property = properties->getNextProperty())
    {
        __aliases[property->name] = properties->getString();
    }
}

std::string FileSystem::displayFileDialog(size_t dialogMode,
                                          const std::string& title,
                                          const std::string& filterDescription,
                                          const std::string& filterExtensions,
                                          const std::string& initialDirectory)
{
    return Platform::displayFileDialog(dialogMode,
                                       title,
                                       filterDescription,
                                       filterExtensions,
                                       initialDirectory);
}

std::string FileSystem::resolvePath(const std::string& path)
{
    size_t len = path.size();
    if (len > 1 && path[0] == '@')
    {
        std::string alias(path.substr(1));
        std::map<std::string, std::string>::const_iterator itr = __aliases.find(alias);
        if (itr == __aliases.end()) return path; // no matching alias found
        return itr->second;
    }

    return path;
}

bool FileSystem::fileExists(const std::string& filePath)
{
    std::string fullPath;
    getFullPath(filePath, fullPath);

    return std::filesystem::exists(fullPath);
}

std::unique_ptr<Stream> FileSystem::open(const std::string& path, size_t streamMode)
{
    char modeStr[] = "rb";
    if ((streamMode & WRITE) != 0) modeStr[0] = 'w';

    std::string fullPath;
    getFullPath(path, fullPath);
    auto stream = FileStream::create(fullPath, modeStr);
    return std::move(stream);
}

char* FileSystem::readAll(const std::string& filePath, int* fileSize)
{
    // Open file for reading.
    std::unique_ptr<Stream> stream(open(filePath));
    if (!stream)
    {
        GP_ERROR("Failed to load file: %s", filePath);
        return nullptr;
    }
    size_t size = stream->length();

    // Read entire file contents.
    char* buffer = new char[size + 1];
    size_t read = stream->read(buffer, 1, size);
    if (read != size)
    {
        GP_ERROR("Failed to read complete contents of file '%s' (amount read vs. file size: %u < "
                 "%u).",
                 filePath,
                 read,
                 size);
        SAFE_DELETE_ARRAY(buffer);
        return nullptr;
    }

    // Force the character buffer to be nullptr-terminated.
    buffer[size] = '\0';

    if (fileSize)
    {
        *fileSize = (int)size;
    }

    return buffer;
}

bool FileSystem::isAbsolutePath(const std::string& filePath)
{
    if (filePath.empty()) return false;

    namespace fs = std::filesystem;
    fs::path p(filePath);
    return p.is_absolute();
}

void FileSystem::setAssetPath(const std::string& path) { __assetPath = path; }

std::string FileSystem::getAssetPath() { return __assetPath; }

void FileSystem::createFileFromAsset(const std::string& path) {}

std::string FileSystem::getDirectoryName(const std::string& path)
{
    if (path.empty()) return EMPTY_STRING;

    namespace fs = std::filesystem;
    fs::path p(path);
    std::string dirname = p.parent_path().string();
    std::replace(dirname.begin(), dirname.end(), '\\', '/');
    dirname.append("/");

    return dirname;
}

std::string FileSystem::getExtension(const std::string& path)
{
    namespace fs = std::filesystem;
    fs::path p(path);
    std::string ext = p.extension().string();

    if (ext.empty()) return EMPTY_STRING;

    // Convert to uppercase
    std::transform(ext.begin(), ext.end(), ext.begin(), [](auto c) { return std::toupper(c); });

    return ext;
}

FileStream::FileStream(std::unique_ptr<std::fstream> stream) : _stream(std::move(stream)) {}

FileStream::~FileStream()
{
    if (_stream && _stream->is_open())
    {
        close();
    }
}

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

    if (!fstream->is_open())
    {
        return nullptr;
    }

    auto stream = std::unique_ptr<FileStream>(new FileStream(std::move(fstream)));

    // Set read/write flags based on the mode
    stream->_canRead = (iosMode & std::ios::in) != 0;
    stream->_canWrite = (iosMode & std::ios::out) != 0;

    return stream;
}

bool FileStream::canRead() { return _stream && _stream->is_open() && _canRead; }

bool FileStream::canWrite() { return _stream && _stream->is_open() && _canWrite; }

bool FileStream::canSeek() { return _stream && _stream->is_open(); }

void FileStream::close()
{
    if (_stream && _stream->is_open()) _stream->close();
}

size_t FileStream::read(void* ptr, size_t size, size_t count)
{
    if (!_stream || !canRead()) return 0;

    _stream->read(static_cast<char*>(ptr), size * count);
    return _stream->gcount() / size;
}

char* FileStream::readLine(char* str, int num)
{
    if (!_stream || !canRead()) return nullptr;

    _stream->getline(str, num);
    return (_stream->gcount() > 0) ? str : nullptr;
}

size_t FileStream::write(const void* ptr, size_t size, size_t count)
{
    if (!_stream || !canWrite()) return 0;

    auto before = _stream->tellp();
    _stream->write(static_cast<const char*>(ptr), size * count);
    auto after = _stream->tellp();

    return (after - before) / size;
}

bool FileStream::eof()
{
    if (!_stream) return true;
    return _stream->eof();
}

size_t FileStream::length()
{
    if (!canSeek()) return 0;

    auto current = _stream->tellg();
    _stream->seekg(0, std::ios::end);
    auto length = _stream->tellg();
    _stream->seekg(current);

    return static_cast<size_t>(length);
}

long int FileStream::position()
{
    if (!_stream) return -1;
    return static_cast<long int>(_stream->tellg());
}

/**
 * @brief Moves the file stream's read and write position to a specified location.
 * @param offset The number of bytes to offset from the origin.
 * @param origin The reference position from which to apply the offset. Should be one of SEEK_SET (beginning), SEEK_CUR (current position), or SEEK_END (end).
 * @return True if the seek operation was successful; otherwise, false.
 */
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

    _stream->seekg(offset, dir);
    return _stream->good();
}

bool FileStream::rewind()
{
    if (!canSeek()) return false;

    _stream->seekg(0, std::ios::beg);
    _stream->clear(); // Clear any error flags

    return true;
}
} // namespace tractor
