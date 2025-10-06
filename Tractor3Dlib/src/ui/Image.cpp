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

#include "ui/Image.h"

#include "framework/FileSystem.h"

namespace tractor
{
// Callback for reading a png image using Stream
static void readStream(png_structp png, png_bytep data, png_size_t length)
{
    Stream* stream = reinterpret_cast<Stream*>(png_get_io_ptr(png));
    if (stream == nullptr || stream->read(data, 1, length) != length)
    {
        png_error(png, "Error reading PNG.");
    }
}

Image* Image::create(const std::string& path)
{
    // Open the file.
    std::unique_ptr<Stream> stream(FileSystem::open(path));
    if (stream.get() == nullptr || !stream->canRead())
    {
        GP_ERROR("Failed to open image file '%s'.", path);
        return nullptr;
    }

    // Verify PNG signature.
    unsigned char sig[8];
    if (stream->read(sig, 1, 8) != 8 || png_sig_cmp(sig, 0, 8) != 0)
    {
        GP_ERROR("Failed to load file '%s'; not a valid PNG.", path);
        return nullptr;
    }

    // Initialize png read struct (last three parameters use stderr+longjump if nullptr).
    png_structp png = png_create_read_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
    if (png == nullptr)
    {
        GP_ERROR("Failed to create PNG structure for reading PNG file '%s'.", path);
        return nullptr;
    }

    // Initialize info struct.
    png_infop info = png_create_info_struct(png);
    if (info == nullptr)
    {
        GP_ERROR("Failed to create PNG info structure for PNG file '%s'.", path);
        png_destroy_read_struct(&png, nullptr, nullptr);
        return nullptr;
    }

    // Set up error handling (required without using custom error handlers above).
    if (setjmp(png_jmpbuf(png)))
    {
        GP_ERROR("Failed to set up error handling for reading PNG file '%s'.", path);
        png_destroy_read_struct(&png, &info, nullptr);
        return nullptr;
    }

    // Initialize file io.
    png_set_read_fn(png, stream.get(), readStream);

    // Indicate that we already read the first 8 bytes (signature).
    png_set_sig_bytes(png, 8);

    // Read the entire image into memory.
    png_read_png(png,
                 info,
                 PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND
                     | PNG_TRANSFORM_GRAY_TO_RGB,
                 nullptr);

    Image* image = new Image();
    image->_width = png_get_image_width(png, info);
    image->_height = png_get_image_height(png, info);

    png_byte colorType = png_get_color_type(png, info);
    switch (colorType)
    {
        case PNG_COLOR_TYPE_RGBA:
            image->_format = Image::RGBA;
            break;

        case PNG_COLOR_TYPE_RGB:
            image->_format = Image::RGB;
            break;

        default:
            GP_ERROR("Unsupported PNG color type (%d) for image file '%s'.", (int)colorType, path);
            png_destroy_read_struct(&png, &info, nullptr);
            return nullptr;
    }

    size_t stride = png_get_rowbytes(png, info);

    // Allocate image data.
    image->_data = new unsigned char[stride * image->_height];

    // Read rows into image data.
    png_bytepp rows = png_get_rows(png, info);
    for (size_t i = 0; i < image->_height; ++i)
    {
        memcpy(image->_data + (stride * (image->_height - 1 - i)), rows[i], stride);
    }

    // Clean up.
    png_destroy_read_struct(&png, &info, nullptr);

    return image;
}

Image* Image::create(unsigned int width, unsigned int height, Image::Format format, unsigned char* data)
{
    assert(width > 0 && height > 0);
    assert(format >= RGB && format <= RGBA);

    unsigned int pixelSize = 0;
    switch (format)
    {
        case Image::RGB:
            pixelSize = 3;
            break;
        case Image::RGBA:
            pixelSize = 4;
            break;
    }

    Image* image = new Image();

    unsigned int dataSize = width * height * pixelSize;

    image->_width = width;
    image->_height = height;
    image->_format = format;
    image->_data = new unsigned char[dataSize];
    if (data) memcpy(image->_data, data, dataSize);

    return image;
}

Image::Image() : _data(nullptr), _format(RGB), _width(0), _height(0) {}

Image::~Image() { SAFE_DELETE_ARRAY(_data); }

} // namespace tractor
