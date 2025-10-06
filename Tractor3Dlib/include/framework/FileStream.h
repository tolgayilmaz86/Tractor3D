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

#include <framework/Stream.h>

#include <memory>

namespace tractor
{
/**
 *
 * @script{ignore}
 */
class FileStream : public Stream
{
  public:
    friend class FileSystem;

    /**
     * @brief Creates a new FileStream object for the specified file and mode.
     * @param filePath The path to the file to open.
     * @param mode The mode in which to open the file (e.g., "r" for read, "w" for write).
     * @return A unique pointer to the created FileStream object.
     */
    static std::unique_ptr<FileStream> create(const std::string& filePath, const std::string& mode);

    /**
     * @brief Destroys the FileStream object and releases any associated resources.
     */
    ~FileStream();

    /**
     * @brief Checks if reading is possible.
     * @return true if reading is possible; otherwise, false.
     */
    virtual bool canRead();

    /**
     * @brief Checks if writing is currently permitted.
     * @return true if writing is allowed; otherwise, false.
     */
    virtual bool canWrite();

    /**
     * @brief Determines whether seeking is supported.
     * @return true if seeking is supported; otherwise, false.
     */
    virtual bool canSeek();

    /**
     * @brief Closes the resource or connection associated with the object.
     */
    virtual void close();

    /**
     * @brief Reads data from a source into a buffer.
     * @param ptr Pointer to the buffer where the read data will be stored.
     * @param size Size in bytes of each element to be read.
     * @param count Number of elements to read.
     * @return The number of elements successfully read.
     */
    virtual size_t read(void* ptr, size_t size, size_t count);

    /**
     * @brief Reads a line from the input stream into the provided buffer.
     * @param str Pointer to the buffer where the read line will be stored.
     * @param num Maximum number of characters to read, including the null terminator.
     * @return Pointer to the buffer containing the read line, or nullptr if end-of-file or an error occurs.
     */
    virtual char* readLine(char* str, int num);

    /**
     * @brief Writes data from a buffer to a destination.
     * @param ptr Pointer to the buffer containing the data to write.
     * @param size Size in bytes of each element to write.
     * @param count Number of elements to write.
     * @return The number of elements successfully written.
     */
    virtual size_t write(const void* ptr, size_t size, size_t count);

    /**
     * @brief Checks whether the end of the input has been reached.
     * @return true if the end of the input has been reached; otherwise, false.
     */
    virtual bool eof();

    /**
     * @brief Returns the number of elements or characters in the object.
     * @return The length of the object, typically as the number of elements or characters.
     */
    virtual size_t length();

    /**
     * @brief Returns the current position indicator.
     * @return The current position as a long integer.
     */
    virtual long int position();

    /**
     * @brief Moves the current position in the stream to a new location, relative to a specified origin.
     * @param offset The number of bytes to move the position by, relative to the origin.
     * @param origin The reference point for the offset. Typically, this is one of SEEK_SET (beginning), SEEK_CUR (current position), or SEEK_END (end).
     * @return true if the seek operation was successful; false otherwise.
     */
    virtual bool seek(long int offset, int origin);

    /**
     * @brief Resets the current position to the beginning.
     * @return true if the operation was successful; otherwise, false.
     */
    virtual bool rewind();

  private:
    /**
     * @brief Constructs a FileStream object that manages the given file stream.
     * @param stream A unique pointer to a std::fstream object to be managed by the FileStream.
     */
    FileStream(std::unique_ptr<std::fstream> stream);

  private:
    std::unique_ptr<std::fstream> _stream;
    bool _canRead{ false };
    bool _canWrite{ false };
};
} // namespace tractor