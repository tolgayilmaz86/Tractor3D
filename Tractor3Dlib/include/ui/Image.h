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

#include "utils/Ref.h"

namespace tractor
{

/**
 * Defines an image buffer of RGB or RGBA color data.
 *
 * Currently only supports loading from .png image files.
 */
class Image : public Ref
{
  public:
    /**
     * Defines the set of supported color formats.
     */
    enum Format
    {
        RGB,
        RGBA
    };

    /**
     * Creates an image from the image file at the given path.
     *
     * @param path The path to the image file.
     * @return The newly created image.
     * @script{create}
     */
    static Image* create(const std::string& path);

    /**
     * Creates an image from the data provided
     *
     * @param width The width of the image data.
     * @param height The height of the image data.
     * @param format The format of the image data.
     * @param data The image data. If nullptr, the data will be allocated.
     * @return The newly created image.
     * @script{create}
     */
    static Image* create(unsigned int width,
                         unsigned int height,
                         Format format,
                         unsigned char* data = nullptr);

    /**
     * Gets the image's raw pixel data.
     *
     * @return The image's pixel data.
     * @script{ignore}
     */
    constexpr unsigned char* getData() const { return _data; }

    /**
     * Gets the image's format.
     *
     * @return The image's format.
     */
    constexpr Image::Format getFormat() const { return _format; }

    /**
     * Gets the height of the image.
     *
     * @return The height of the image.
     */
    constexpr unsigned int getHeight() const { return _height; }

    /**
     * Gets the width of the image.
     *
     * @return The width of the image.
     */
    constexpr unsigned int getWidth() const { return _width; }

  private:
    /**
     * Constructor.
     */
    Image();

    /**
     * Destructor.
     */
    ~Image();

    /**
     * Hidden copy assignment operator.
     */
    Image& operator=(const Image&) = delete;

    unsigned char* _data;
    Format _format;
    unsigned int _width;
    unsigned int _height;
};

} // namespace tractor
