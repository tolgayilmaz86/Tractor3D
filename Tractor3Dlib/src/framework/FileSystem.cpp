#include "pch.h"

#include <framework/FileStream.h>
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

    if (fileSize) *fileSize = (int)size;

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
} // namespace tractor
