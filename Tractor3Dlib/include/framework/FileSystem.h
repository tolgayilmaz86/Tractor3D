#pragma once

#include <string>

#include "framework/Stream.h"

namespace tractor
{

class Properties;

/**
 * Defines a set of functions for interacting with the device file system.
 */
class FileSystem
{
  public:
    /**
     * Mode flags for opening a stream.
     *
     * @script{ignore}
     */
    enum StreamMode
    {
        READ = 1,
        WRITE = 2
    };

    /**
     * Mode flags for displaying a dialog.
     *
     * @script{ignore}
     */
    enum DialogMode
    {
        OPEN,
        SAVE
    };

    /**
     * Destructor.
     */
    ~FileSystem();

    /**
     * Sets the path to the root of the resources folder for the game.
     *
     * Once set, all resource/file loading will load from the given path.
     * The default resource path is "./".
     *
     * @param path The path to the root of the resources folder.
     */
    static void setResourcePath(const std::string& path);

    /**
     * Returns the currently set resource path.
     *
     * @return The currently set resource path.
     */
    static std::string getResourcePath();

    /**
     * Loads a properties file containing a list of filesystem aliases.
     *
     * The specified aliases file is a valid properties file that contains one
     * or more namespaces with a list of filesystem aliases that will be used
     * to establish soft links to files when reading files through this class.
     *
     * This can be helpful for managing loading of resources that may change
     * from one platform to another (such as texture formats). An aliases
     * file per-platform can be maintained and asset loading code can refer
     * to the alias name instead of the actual hard file name.
     *
     * @param aliasFilePath Path to a properties file containing filesystem aliases.
     *
     * @see Properties
     */
    static void loadResourceAliases(const std::string& aliasFilePath);

    /**
     * Loads a set of filesystem aliases from the given Properties object.
     *
     * The specified properties object contains a single namespace with a list
     * of filesystem aliases that will be used to establish soft links to files
     * when reading files through this class.
     *
     * This can be helpful for managing loading of resources that may change
     * from one platform to another (such as texture formats). An aliases
     * file per-platform can be maintained and asset loading code can refer
     * to the alias name instead of the actual hard file name.
     *
     * @param properties Properties object containing filesystem aliases.
     *
     * @see Properties
     */
    static void loadResourceAliases(Properties* properties);

    /**
     * Displays an open or save dialog using the native platform dialog system.
     *
     * @param dialogMode The mode of the dialog. (Ex. OPEN or SAVE)
     * @param title The title of the dialog. (Ex. Select File or Save File)
     * @param filterDescription The file filter description. (Ex. All Files or Image Files)
     * @param filterExtensions The extensions to filter on. (Ex. png;bmp)
     * @param initialDirectory The initial directory to start. nullptr runs from the executable directory.
     * @return The file that is opened or saved, or an empty string if canceled.
     *
     * @script{ignore}
     */
    static std::string displayFileDialog(size_t dialogMode,
                                         const std::string& title,
                                         const std::string& filterDescription,
                                         const std::string& filterExtensions,
                                         const std::string& initialDirectory);

    /**
     * Resolves a filesystem path.
     *
     * If the specified path is a filesystem alias, the alias will be
     * resolved and the physical file will be returned.
     *
     * Note that this method does not convert a relative path to an
     * absolute filesystem path.
     *
     * @param path Path to resolve.
     *
     * @return The resolved file path.
     */
    static std::string resolvePath(const std::string& path);

    /**
     * Lists the files in the specified directory and adds the files to the vector. Excludes directories.
     *
     * @param dirPath Directory path relative to the path set in <code>setResourcePath(const std::string&)</code>.
     * @param files The vector to append the files to.
     *
     * @return True if successful, false if error.
     *
     * @script{ignore}
     */
    static bool listFiles(const std::string& dirPath, std::vector<std::string>& files);

    /**
     * Checks if the file at the given path exists.
     *
     * @param filePath The path to the file.
     *
     * @return <code>true</code> if the file exists; <code>false</code> otherwise.
     */
    static bool fileExists(const std::string& filePath);

    /**
     * Opens a byte stream for the given resource path.
     *
     * If <code>path</code> is a file path, the file at the specified location is opened relative to
     * the currently set resource path.
     *
     * @param path The path to the resource to be opened, relative to the currently set resource
     * path.
     * @param streamMode The stream mode used to open the file.
     *
     * @return A stream that can be used to read or write to the file depending on the mode.
     *         Returns nullptr if there was an error. (Request mode not supported).
     *
     * @script{ignore}
     */
    static std::unique_ptr<Stream> open(const std::string& path, size_t streamMode = READ);

    /**
     * Opens the specified file.
     *
     * The file at the specified location is opened, relative to the currently set
     * resource path.
     *
     * @param filePath The path to the file to be opened, relative to the currently set resource path.
     * @param mode The mode used to open the file, passed directly to fopen.
     *
     * @return A pointer to a FILE object that can be used to identify the stream or nullptr on error.
     *
     * @see setResourcePath(const std::string&)
     * @script{ignore}
     */
    static FILE* openFile(const std::string& filePath, const std::string& mode);

    /**
     * Reads the entire contents of the specified file and returns its contents.
     *
     * The returned character array is allocated with new[] and must therefore
     * deleted by the caller using delete[].
     *
     * @param filePath The path to the file to be read.
     * @param fileSize The size of the file in bytes (optional).
     *
     * @return A newly allocated (nullptr-terminated) character array containing the
     *      contents of the file, or nullptr if the file could not be read.
     */
    static char* readAll(const std::string& filePath, int* fileSize = nullptr);

    /**
     * Determines if the file path is an absolute path for the current platform.
     *
     * @param filePath The file path to test.
     *
     * @return True if the path is an absolute path or false otherwise.
     */
    static bool isAbsolutePath(const std::string& filePath);

    /**
     * Sets the asset root path for the game on platforms that have separate assets (currently just Android).
     *
     * Once set, all asset paths will be loaded relative to the given path.
     * The default asset path is an empty string ("").
     *
     * @param path The asset root path.
     */
    static void setAssetPath(const std::string& path);

    /**
     * Returns the currently set asset root path.
     *
     * @return The currently set asset root path.
     */
    static std::string getAssetPath();

    /**
     * Creates a file on the file system from the specified asset (Android-specific).
     *
     * @param path The path to the file.
     */
    static void createFileFromAsset(const std::string& path);

    /**
     * Returns the directory name up to and including the trailing '/'.
     *
     * This is a lexical method so it does not verify that the directory exists.
     * Back slashes will be converted to forward slashes.
     *
     * - "res/image.png" will return "res/"
     * - "image.png" will return ""
     * - "c:\foo\bar\image.png" will return "c:/foo/bar/"
     *
     * @param path The file path. May be relative or absolute, forward or back slashes. May be
     * nullptr.
     *
     * @return The directory name with the trailing '/'. Returns "" if path is nullptr or the path
     * does not contain a directory.
     */
    static std::string getDirectoryName(const std::string& path);

    /**
     * Returns the extension of the given file path.
     *
     * The extension returned includes all character after and including the last '.'
     * in the file path. The extension is returned as all uppercase.
     *
     * If the path does not contain an extension, an empty string is returned.
     *
     * @param path File path.
     *
     * @return The file extension, all uppercase, including the '.'.
     */
    static std::string getExtension(const std::string& path);

  private:
    /**
     * Constructor.
     */
    FileSystem();
};

} // namespace tractor
