#include "pch.h"

#include <framework/FileSystem.h>

#include <scene/Properties.h>
#include <framework/Stream.h>
#include <framework/Platform.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <windows.h>
#include <tchar.h>
#include <stdio.h>
#include <direct.h>
#define gp_stat _stat
#define gp_stat_struct struct stat

namespace tractor
{

	/** @script{ignore} */
	static std::string __resourcePath("./");
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
			fullPath.assign(__resourcePath);
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

		~FileStream() = default; // Default destructor, no explicit close call needed
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
		FileStream(FILE* file);

	private:
		struct FileCloser
		{
			void operator()(FILE* file) const
			{
				if (file)
				{
					fclose(file);
				}
			}
		};

		std::unique_ptr<FILE, FileCloser> _file;
		bool _canRead{ false };
		bool _canWrite{ false };
	};

	/////////////////////////////

	FileSystem::FileSystem()
	{
	}

	FileSystem::~FileSystem()
	{
	}

	void FileSystem::setResourcePath(const std::string& path)
	{
		__resourcePath = path.empty() ? "" : path;
	}

	std::string FileSystem::getResourcePath()
	{
		return __resourcePath;
	}

	void FileSystem::loadResourceAliases(const std::string& aliasFilePath)
	{
		Properties* properties = Properties::create(aliasFilePath);
		if (properties)
		{
			Properties* aliases;
			while ((aliases = properties->getNextNamespace()) != nullptr)
			{
				loadResourceAliases(aliases);
			}
		}
		SAFE_DELETE(properties);
	}

	void FileSystem::loadResourceAliases(Properties* properties)
	{
		assert(properties);

		while (auto property = properties->getNextProperty())
		{
			__aliases[property->name] = properties->getString();
		}
	}

	std::string FileSystem::displayFileDialog(size_t dialogMode, const std::string& title, const std::string& filterDescription, const std::string& filterExtensions, const std::string& initialDirectory)
	{
		return Platform::displayFileDialog(dialogMode, title, filterDescription, filterExtensions, initialDirectory);
	}

	std::string FileSystem::resolvePath(const std::string& path)
	{
		size_t len = path.size();
		if (len > 1 && path[0] == '@')
		{
			std::string alias(path.substr(1));
			std::map<std::string, std::string>::const_iterator itr = __aliases.find(alias);
			if (itr == __aliases.end())
				return path; // no matching alias found
			return itr->second;
		}

		return path;
	}

	bool FileSystem::listFiles(const std::string& dirPath, std::vector<std::string>& files)
	{
		std::string path(FileSystem::getResourcePath());
		if (not dirPath.empty())
		{
			path.append(dirPath);
		}

		path.append("/*");
		// Convert char to wchar
		std::basic_string<TCHAR> wPath;
		wPath.assign(path.begin(), path.end());

		WIN32_FIND_DATA FindFileData;
		HANDLE hFind = FindFirstFile(wPath.c_str(), &FindFileData);
		if (hFind == INVALID_HANDLE_VALUE)
		{
			return false;
		}
		do
		{
			// Add to the list if this is not a directory
			if ((FindFileData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) == 0)
			{
				// Convert wchar to char
				std::basic_string<TCHAR> wfilename(FindFileData.cFileName);
				std::string filename;
				filename.assign(wfilename.begin(), wfilename.end());
				files.push_back(filename);
			}
		} while (FindNextFile(hFind, &FindFileData) != 0);

		FindClose(hFind);
		return true;
	}

	bool FileSystem::fileExists(const std::string& filePath)
	{
		std::string fullPath;
		getFullPath(filePath, fullPath);

		gp_stat_struct s;
		return stat(fullPath.c_str(), &s) == 0;

	}

	std::unique_ptr<Stream> FileSystem::open(const std::string& path, size_t streamMode)
	{
		char modeStr[] = "rb";
		if ((streamMode & WRITE) != 0)
			modeStr[0] = 'w';

		std::string fullPath;
		getFullPath(path, fullPath);
		auto stream = FileStream::create(fullPath.c_str(), modeStr);
		return std::move(stream);
	}

	FILE* FileSystem::openFile(const std::string& filePath, const std::string& mode)
	{
		std::string fullPath;
		getFullPath(filePath, fullPath);

		createFileFromAsset(filePath);

		FILE* fp = fopen(fullPath.c_str(), mode.c_str());
		return fp;
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
			GP_ERROR("Failed to read complete contents of file '%s' (amount read vs. file size: %u < %u).", filePath, read, size);
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
		if (filePath.empty())
			return false;

		if (filePath[1] != '\0')
		{
			char first = filePath[0];
			return (filePath[1] == ':' && ((first >= 'a' && first <= 'z') || (first >= 'A' && first <= 'Z')));
		}

		return false;
	}

	void FileSystem::setAssetPath(const std::string& path)
	{
		__assetPath = path;
	}

	std::string FileSystem::getAssetPath()
	{
		return __assetPath;
	}

	void FileSystem::createFileFromAsset(const std::string& path)
	{
	}

	std::string FileSystem::getDirectoryName(const std::string& path)
	{
		if (path.empty())
			return "";

		char drive[_MAX_DRIVE];
		char dir[_MAX_DIR];
		_splitpath(path.c_str(), drive, dir, nullptr, nullptr);

		std::string dirname;
		size_t driveLength = strlen(drive);
		if (driveLength > 0)
		{
			dirname.reserve(driveLength + strlen(dir));
			dirname.append(drive);
			dirname.append(dir);
		}
		else
		{
			dirname.assign(dir);
		}
		std::replace(dirname.begin(), dirname.end(), '\\', '/');
		return dirname;
	}

	std::string FileSystem::getExtension(const std::string& path)
	{
		// Find the last occurrence of '.' in the path
		auto pos = path.find_last_of('.');
		if (pos == std::string::npos || pos == path.length() - 1)
		{
			// No '.' found or '.' is the last character (no extension)
			return "";
		}

		// Extract the extension (including the '.')
		std::string ext = path.substr(pos);

		// Convert the extension to uppercase
		std::transform(ext.begin(), ext.end(), ext.begin(),
			[](auto c) { return std::tolower(c); });

		return ext;
	}

	FileStream::FileStream(FILE* file)
		: _file(file)
	{
	}

	std::unique_ptr<FileStream> FileStream::create(const std::string& filePath, const std::string& mode)
	{
		FILE* file = fopen(filePath.c_str(), mode.c_str());

		if (!file)
			return nullptr;

		auto stream = std::unique_ptr<FileStream>(new FileStream(file));

		// Set read/write flags based on the mode string
		stream->_canRead = mode.find('r') != std::string::npos;
		stream->_canWrite = mode.find('w') != std::string::npos;

		return stream;
	}

	bool FileStream::canRead()
	{
		return _file && _canRead;
	}

	bool FileStream::canWrite()
	{
		return _file && _canWrite;
	}

	bool FileStream::canSeek()
	{
		return _file != nullptr;
	}

	void FileStream::close()
	{
		_file.reset(); // Explicitly reset the unique_ptr to close the file
	}

	size_t FileStream::read(void* ptr, size_t size, size_t count)
	{
		if (!_file)
			return 0;
		return fread(ptr, size, count, _file.get());
	}

	char* FileStream::readLine(char* str, int num)
	{
		if (!_file)
			return 0;
		return fgets(str, num, _file.get());
	}

	size_t FileStream::write(const void* ptr, size_t size, size_t count)
	{
		if (!_file)
			return 0;
		return fwrite(ptr, size, count, _file.get());
	}

	bool FileStream::eof()
	{
		if (!_file || feof(_file.get()))
			return true;
		return ((size_t)position()) >= length();
	}

	size_t FileStream::length()
	{
		size_t len = 0;
		if (canSeek())
		{
			long int pos = position();
			if (seek(0, SEEK_END))
			{
				len = position();
			}
			seek(pos, SEEK_SET);
		}
		return len;
	}

	long int FileStream::position()
	{
		if (!_file)
			return -1;
		return ftell(_file.get());
	}

	bool FileStream::seek(long int offset, int origin)
	{
		if (!_file)
			return false;
		return fseek(_file.get(), offset, origin) == 0;
	}

	bool FileStream::rewind()
	{
		if (canSeek())
		{
			::rewind(_file.get());
			return true;
		}
		return false;
	}
}
