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

#include "renderer/Texture.h"

#include <filesystem>

#include "framework/FileSystem.h"
#include "ui/Image.h"

// PVRTC (GL_IMG_texture_compression_pvrtc) : Imagination based gpus
#ifndef GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG 0x8C01
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG 0x8C03
#endif
#ifndef GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG 0x8C00
#endif
#ifndef GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
#define GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG 0x8C02
#endif

// S3TC/DXT (GL_EXT_texture_compression_s3tc) : Most desktop/console gpus
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT1_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT1_EXT 0x83F1
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT3_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT3_EXT 0x83F2
#endif
#ifndef GL_COMPRESSED_RGBA_S3TC_DXT5_EXT
#define GL_COMPRESSED_RGBA_S3TC_DXT5_EXT 0x83F3
#endif

// ATC (GL_AMD_compressed_ATC_texture) : Qualcomm/Adreno based gpus
#ifndef ATC_RGB_AMD
#define ATC_RGB_AMD 0x8C92
#endif
#ifndef ATC_RGBA_EXPLICIT_ALPHA_AMD
#define ATC_RGBA_EXPLICIT_ALPHA_AMD 0x8C93
#endif
#ifndef ATC_RGBA_INTERPOLATED_ALPHA_AMD
#define ATC_RGBA_INTERPOLATED_ALPHA_AMD 0x87EE
#endif

// ETC1 (OES_compressed_ETC1_RGB8_texture) : All OpenGL ES chipsets
#ifndef ETC1_RGB8
#define ETC1_RGB8 0x8D64
#endif

namespace tractor
{

static std::vector<Texture*> __textureCache;
static TextureHandle __currentTextureId = 0;
static Texture::Type __currentTextureType = Texture::TEXTURE_2D;

Texture::Texture()
    : _handle(0), _format(UNKNOWN), _type((Texture::Type)0), _width(0), _height(0),
      _mipmapped(false), _cached(false), _compressed(false), _wrapS(Texture::REPEAT),
      _wrapT(Texture::REPEAT), _wrapR(Texture::REPEAT), _minFilter(Texture::NEAREST_MIPMAP_LINEAR),
      _magFilter(Texture::LINEAR)
{
}

Texture::~Texture()
{
    if (_handle)
    {
        GL_ASSERT(glDeleteTextures(1, &_handle));
        _handle = 0;
    }

    // Remove ourself from the texture cache.
    if (_cached)
    {
        std::vector<Texture*>::iterator itr =
            std::find(__textureCache.begin(), __textureCache.end(), this);
        if (itr != __textureCache.end())
        {
            __textureCache.erase(itr);
        }
    }
}

Texture* Texture::create(const std::string& path, bool generateMipmaps)
{
    // Search texture cache first.
    for (size_t i = 0, count = __textureCache.size(); i < count; ++i)
    {
        Texture* t = __textureCache[i];
        assert(t);
        if (t->_path == path)
        {
            // If 'generateMipmaps' is true, call Texture::generateMipamps() to force the
            // texture to generate its mipmap chain if it hasn't already done so.
            if (generateMipmaps)
            {
                t->generateMipmaps();
            }

            // Found a match.
            t->addRef();

            return t;
        }
    }

    Texture* texture = nullptr;

    // Filter loading based on file extension.
    std::filesystem::path filePath(FileSystem::resolvePath(path));
    std::string ext = filePath.extension().string();

    std::transform(ext.begin(), ext.end(), ext.begin(), [](auto c) { return std::tolower(c); });

    if (ext == ".png")
    {
        Image* image = Image::create(path);
        if (image) texture = create(image, generateMipmaps);
        SAFE_RELEASE(image);
    }
    else if (ext == ".pvr")
    {
        // PowerVR Compressed Texture RGBA.
        texture = createCompressedPVRTC(path);
    }
    else if (ext == ".dds")
    {
        // DDS file format (DXT/S3TC) compressed textures
        texture = createCompressedDDS(path);
    }

    if (texture)
    {
        texture->_path = path;
        texture->_cached = true;

        // Add to texture cache.
        __textureCache.push_back(texture);

        return texture;
    }

    GP_ERROR("Failed to load texture from file '%s'.", path);
    return nullptr;
}

Texture* Texture::create(Image* image, bool generateMipmaps)
{
    assert(image);

    switch (image->getFormat())
    {
        case Image::RGB:
            return create(Texture::RGB,
                          image->getWidth(),
                          image->getHeight(),
                          image->getData(),
                          generateMipmaps);
        case Image::RGBA:
            return create(Texture::RGBA,
                          image->getWidth(),
                          image->getHeight(),
                          image->getData(),
                          generateMipmaps);
        default:
            GP_ERROR("Unsupported image format (%d).", image->getFormat());
            return nullptr;
    }
}

GLint Texture::getFormatInternal(Format format)
{
    switch (format)
    {
        case Texture::RGB888:
        case Texture::RGB565:
            return GL_RGB;
        case Texture::RGBA8888:
        case Texture::RGBA4444:
        case Texture::RGBA5551:
            return GL_RGBA;
        case Texture::ALPHA:
            return GL_ALPHA;
        case Texture::DEPTH:
#if !defined(OPENGL_ES) || defined(GL_ES_VERSION_3_0)
            return GL_DEPTH_COMPONENT32F;
#else
            return GL_DEPTH_COMPONENT;
#endif
        default:
            return 0;
    }
}

GLenum Texture::getFormatTexel(Format format)
{
    switch (format)
    {
        case Texture::RGB888:
        case Texture::RGBA8888:
        case Texture::ALPHA:
            return GL_UNSIGNED_BYTE;
        case Texture::RGB565:
            return GL_UNSIGNED_SHORT_5_6_5;
        case Texture::RGBA4444:
            return GL_UNSIGNED_SHORT_4_4_4_4;
        case Texture::RGBA5551:
            return GL_UNSIGNED_SHORT_5_5_5_1;
        case Texture::DEPTH:
#if !defined(OPENGL_ES) || defined(GL_ES_VERSION_3_0)
            return GL_FLOAT;
#else
            return GL_UNSIGNED_INT;
#endif
        default:
            return 0;
    }
}

size_t Texture::getFormatBPP(Format format)
{
    switch (format)
    {
        case Texture::RGB565:
        case Texture::RGBA4444:
        case Texture::RGBA5551:
            return 2;
        case Texture::RGB888:
            return 3;
        case Texture::RGBA8888:
            return 4;
        case Texture::ALPHA:
            return 1;
        default:
            return 0;
    }
}

Texture* Texture::create(Format format,
                         unsigned int width,
                         unsigned int height,
                         const unsigned char* data,
                         bool generateMipmaps,
                         Texture::Type type)
{
    assert(type == Texture::TEXTURE_2D || type == Texture::TEXTURE_CUBE);

    GLenum target = (GLenum)type;

    GLint internalFormat = getFormatInternal(format);
    assert(internalFormat != 0);

    GLenum texelType = getFormatTexel(format);
    assert(texelType != 0);

    // Create the texture.
    GLuint textureId;
    GL_ASSERT(glGenTextures(1, &textureId));
    GL_ASSERT(glBindTexture(target, textureId));
    GL_ASSERT(glPixelStorei(GL_UNPACK_ALIGNMENT, 1));
    // glGenerateMipmap is new in OpenGL 3.0. For OpenGL 2.0 we must fallback to use glTexParameteri
    // with GL_GENERATE_MIPMAP prior to actual texture creation (glTexImage2D)
    if (generateMipmaps && !std::addressof(glGenerateMipmap))
        GL_ASSERT(glTexParameteri(target, GL_GENERATE_MIPMAP, GL_TRUE));

    // Load the texture
    size_t bpp = getFormatBPP(format);
    if (type == Texture::TEXTURE_2D)
    {
        GLenum f = (format == Texture::DEPTH) ? GL_DEPTH_COMPONENT : internalFormat;
        GL_ASSERT(
            glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, f, texelType, data));
    }
    else
    {
        // Get texture size
        unsigned int textureSize = width * height;
        if (bpp == 0)
        {
            glDeleteTextures(1, &textureId);
            GP_ERROR("Failed to determine texture size because format is UNKNOWN.");
            return nullptr;
        }
        textureSize *= bpp;
        // Texture Cube
        for (size_t i = 0; i < 6; i++)
        {
            const unsigned char* texturePtr = (data == nullptr) ? nullptr : &data[i * textureSize];
            GL_ASSERT(glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                   0,
                                   internalFormat,
                                   width,
                                   height,
                                   0,
                                   internalFormat,
                                   texelType,
                                   texturePtr));
        }
    }

    // Set initial minification filter based on whether or not mipmaping was enabled.
    Filter minFilter;
    if (format == Texture::DEPTH)
    {
        minFilter = NEAREST;
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, GL_NEAREST));
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, GL_NEAREST));
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE));
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE));
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_COMPARE_MODE, GL_NONE));
    }
    else
    {
        minFilter = generateMipmaps ? NEAREST_MIPMAP_LINEAR : LINEAR;
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter));
    }

    Texture* texture = new Texture();
    texture->_handle = textureId;
    texture->_format = format;
    texture->_type = type;
    texture->_width = width;
    texture->_height = height;
    texture->_minFilter = minFilter;
    texture->_internalFormat = internalFormat;
    texture->_texelType = texelType;
    texture->_bpp = bpp;
    if (generateMipmaps) texture->generateMipmaps();

    // Restore the texture id
    GL_ASSERT(glBindTexture((GLenum)__currentTextureType, __currentTextureId));

    return texture;
}

Texture* Texture::create(TextureHandle handle, int width, int height, Format format)
{
    assert(handle);

    Texture* texture = new Texture();
    if (glIsTexture(handle))
    {
        // There is no real way to query for texture type, but an error will be returned if a cube
        // texture is bound to a 2D texture... so check for that
        glBindTexture(GL_TEXTURE_CUBE_MAP, handle);
        if (glGetError() == GL_NO_ERROR)
        {
            texture->_type = TEXTURE_CUBE;
        }
        else
        {
            // For now, it's either or. But if 3D textures and others are added, it might be useful
            // to simply test a bunch of bindings and seeing which one doesn't error out
            texture->_type = TEXTURE_2D;
        }

        // Restore the texture id
        GL_ASSERT(glBindTexture((GLenum)__currentTextureType, __currentTextureId));
    }
    texture->_handle = handle;
    texture->_format = format;
    texture->_width = width;
    texture->_height = height;
    texture->_internalFormat = getFormatInternal(format);
    texture->_texelType = getFormatTexel(format);
    texture->_bpp = getFormatBPP(format);

    return texture;
}

void Texture::setData(const unsigned char* data)
{
    // Don't work with any compressed or cached textures
    assert(data);
    assert((!_compressed));
    assert((!_cached));

    GL_ASSERT(glBindTexture((GLenum)_type, _handle));

    if (_type == Texture::TEXTURE_2D)
    {
        GL_ASSERT(
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, _width, _height, _internalFormat, _texelType, data));
    }
    else
    {
        // Get texture size
        unsigned int textureSize = _width * _height;
        textureSize *= _bpp;
        // Texture Cube
        for (size_t i = 0; i < 6; i++)
        {
            GL_ASSERT(glTexSubImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
                                      0,
                                      0,
                                      0,
                                      _width,
                                      _height,
                                      _internalFormat,
                                      _texelType,
                                      &data[i * textureSize]));
        }
    }

    if (_mipmapped)
    {
        generateMipmaps();
    }

    // Restore the texture id
    GL_ASSERT(glBindTexture((GLenum)__currentTextureType, __currentTextureId));
}

// Computes the size of a PVRTC data chunk for a mipmap level of the given size.
static unsigned int computePVRTCDataSize(int width, int height, int bpp)
{
    int blockSize;
    int widthBlocks;
    int heightBlocks;

    if (bpp == 4)
    {
        blockSize = 4 * 4; // Pixel by pixel block size for 4bpp
        widthBlocks = std::max(width >> 2, 2);
        heightBlocks = std::max(height >> 2, 2);
    }
    else
    {
        blockSize = 8 * 4; // Pixel by pixel block size for 2bpp
        widthBlocks = std::max(width >> 3, 2);
        heightBlocks = std::max(height >> 2, 2);
    }

    return widthBlocks * heightBlocks * ((blockSize * bpp) >> 3);
}

Texture* Texture::createCompressedPVRTC(const std::string& path)
{
    std::unique_ptr<Stream> stream(FileSystem::open(path));
    if (stream.get() == nullptr || !stream->canRead())
    {
        GP_ERROR("Failed to load file '%s'.", path);
        return nullptr;
    }

    // Read first 4 bytes to determine PVRTC format.
    size_t read;
    unsigned int version;
    read = stream->read(&version, sizeof(unsigned int), 1);
    if (read != 1)
    {
        GP_ERROR("Failed to read PVR version.");
        return nullptr;
    }

    // Rewind to start of header.
    if (stream->seek(0, SEEK_SET) == false)
    {
        GP_ERROR("Failed to seek backwards to beginning of file after reading PVR version.");
        return nullptr;
    }

    // Read texture data.
    GLsizei width, height;
    GLenum format;
    GLubyte* data = nullptr;
    unsigned int mipMapCount;
    unsigned int faceCount;
    GLenum faces[6] = { GL_TEXTURE_2D };

    if (version == 0x03525650)
    {
        // Modern PVR file format.
        data = readCompressedPVRTC(path,
                                   stream.get(),
                                   &width,
                                   &height,
                                   &format,
                                   &mipMapCount,
                                   &faceCount,
                                   faces);
    }
    else
    {
        // Legacy PVR file format.
        data = readCompressedPVRTCLegacy(path,
                                         stream.get(),
                                         &width,
                                         &height,
                                         &format,
                                         &mipMapCount,
                                         &faceCount,
                                         faces);
    }
    if (data == nullptr)
    {
        GP_ERROR("Failed to read texture data from PVR file '%s'.", path);
        return nullptr;
    }
    stream->close();

    int bpp = (format == GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG
               || format == GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG)
                  ? 2
                  : 4;

    // Generate our texture.
    GLenum target = faceCount > 1 ? GL_TEXTURE_CUBE_MAP : GL_TEXTURE_2D;
    GLuint textureId;
    GL_ASSERT(glGenTextures(1, &textureId));
    GL_ASSERT(glBindTexture(target, textureId));

    Filter minFilter = mipMapCount > 1 ? NEAREST_MIPMAP_LINEAR : LINEAR;
    GL_ASSERT(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter));

    Texture* texture = new Texture();
    texture->_handle = textureId;
    texture->_type = faceCount > 1 ? TEXTURE_CUBE : TEXTURE_2D;
    texture->_width = width;
    texture->_height = height;
    texture->_mipmapped = mipMapCount > 1;
    texture->_compressed = true;
    texture->_minFilter = minFilter;

    // Load the data for each level.
    GLubyte* ptr = data;
    for (size_t level = 0; level < mipMapCount; ++level)
    {
        unsigned int dataSize = computePVRTCDataSize(width, height, bpp);

        for (size_t face = 0; face < faceCount; ++face)
        {
            // Upload data to GL.
            GL_ASSERT(glCompressedTexImage2D(faces[face],
                                             level,
                                             format,
                                             width,
                                             height,
                                             0,
                                             dataSize,
                                             &ptr[face * dataSize]));
        }

        width = std::max(width >> 1, 1);
        height = std::max(height >> 1, 1);
        ptr += dataSize * faceCount;
    }

    // Free data.
    SAFE_DELETE_ARRAY(data);

    // Restore the texture id
    GL_ASSERT(glBindTexture((GLenum)__currentTextureType, __currentTextureId));

    return texture;
}

GLubyte* Texture::readCompressedPVRTC(const std::string& path,
                                      Stream* stream,
                                      GLsizei* width,
                                      GLsizei* height,
                                      GLenum* format,
                                      unsigned int* mipMapCount,
                                      unsigned int* faceCount,
                                      GLenum* faces)
{
    assert(stream);
    assert(width);
    assert(height);
    assert(format);
    assert(mipMapCount);
    assert(faceCount);
    assert(faces);

    struct pvrtc_file_header
    {
        unsigned int version;
        unsigned int flags;
        unsigned int pixelFormat[2];
        unsigned int colorSpace;
        unsigned int channelType;
        unsigned int height;
        unsigned int width;
        unsigned int depth;
        unsigned int surfaceCount;
        unsigned int faceCount;
        unsigned int mipMapCount;
        unsigned int metaDataSize;
    };

    struct pvrtc_metadata
    {
        char fourCC[4];
        unsigned int key;
        unsigned int dataSize;
    };

    size_t read;

    // Read header data.
    pvrtc_file_header header;
    read = stream->read(&header, sizeof(pvrtc_file_header), 1);
    if (read != 1)
    {
        GP_ERROR("Failed to read PVR header data for file '%s'.", path);
        return nullptr;
    }

    if (header.pixelFormat[1] != 0)
    {
        // Unsupported pixel format.
        GP_ERROR("Unsupported pixel format in PVR file '%s'. (MSB == %d != 0)",
                 path,
                 header.pixelFormat[1]);
        return nullptr;
    }

    int bpp;
    switch (header.pixelFormat[0])
    {
        case 0:
            *format = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
            bpp = 2;
            break;
        case 1:
            *format = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
            bpp = 2;
            break;
        case 2:
            *format = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
            bpp = 4;
            break;
        case 3:
            *format = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
            bpp = 4;
            break;
        default:
            // Unsupported format.
            GP_ERROR("Unsupported pixel format value (%d) in PVR file '%s'.",
                     header.pixelFormat[0],
                     path);
            return nullptr;
    }

    *width = (GLsizei)header.width;
    *height = (GLsizei)header.height;
    *mipMapCount = header.mipMapCount;
    *faceCount = std::min(header.faceCount, 6u);

    if ((*faceCount) > 1)
    {
        // Look for cubemap metadata and setup faces
        unsigned int remainingMetadata = header.metaDataSize;
        pvrtc_metadata mdHeader;
        bool foundTextureCubeMeta = false;
        while (remainingMetadata > 0)
        {
            read = stream->read(&mdHeader, sizeof(pvrtc_metadata), 1);
            if (read != 1)
            {
                GP_ERROR("Failed to read PVR metadata header data for file '%s'.", path);
                return nullptr;
            }
            remainingMetadata -= sizeof(pvrtc_metadata) + mdHeader.dataSize;

            // Check that it's a known metadata type (specifically, cubemap order), otherwise skip to next metadata
            if ((mdHeader.fourCC[0] != 'P') || (mdHeader.fourCC[1] != 'V')
                || (mdHeader.fourCC[2] != 'R') || (mdHeader.fourCC[3] != 3) || (mdHeader.key != 2)
                || // Everything except cubemap order (cubemap order key is 2)
                (mdHeader.dataSize != 6)) // Cubemap order datasize should be 6
            {
                if (stream->seek(mdHeader.dataSize, SEEK_CUR) == false)
                {
                    GP_ERROR("Failed to seek to next meta data header in PVR file '%s'.", path);
                    return nullptr;
                }
                continue;
            }

            // Get cubemap order
            foundTextureCubeMeta = true;
            char faceOrder[6];
            read = stream->read(faceOrder, 1, sizeof(faceOrder));
            if (read != sizeof(faceOrder))
            {
                GP_ERROR("Failed to read cubemap face order meta data for file '%s'.", path);
                return nullptr;
            }
            for (size_t face = 0; face < (*faceCount); ++face)
            {
                faces[face] = GL_TEXTURE_CUBE_MAP_POSITIVE_X
                              + (faceOrder[face] <= 'Z' ? ((faceOrder[face] - 'X') * 2)
                                                        : (((faceOrder[face] - 'x') * 2) + 1));
                if (faces[face] < GL_TEXTURE_CUBE_MAP_POSITIVE_X)
                {
                    // Just overwrite this face
                    faces[face] = GL_TEXTURE_CUBE_MAP_POSITIVE_X;
                }
            }
        }
        if (!foundTextureCubeMeta)
        {
            // Didn't find cubemap metadata. Just assume it's "in order"
            for (size_t face = 0; face < (*faceCount); ++face)
            {
                faces[face] = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
            }
        }
    }
    else
    {
        // Skip meta-data.
        if (stream->seek(header.metaDataSize, SEEK_CUR) == false)
        {
            GP_ERROR("Failed to seek past header meta data in PVR file '%s'.", path);
            return nullptr;
        }
    }

    // Compute total size of data to be read.
    int w = *width;
    int h = *height;
    size_t dataSize = 0;
    for (size_t level = 0; level < header.mipMapCount; ++level)
    {
        dataSize += computePVRTCDataSize(w, h, bpp) * (*faceCount);
        w = std::max(w >> 1, 1);
        h = std::max(h >> 1, 1);
    }

    // Read data.
    GLubyte* data = new GLubyte[dataSize];
    read = stream->read(data, 1, dataSize);
    if (read != dataSize)
    {
        SAFE_DELETE_ARRAY(data);
        GP_ERROR("Failed to read texture data from PVR file '%s'.", path);
        return nullptr;
    }

    return data;
}

GLubyte* Texture::readCompressedPVRTCLegacy(const std::string& path,
                                            Stream* stream,
                                            GLsizei* width,
                                            GLsizei* height,
                                            GLenum* format,
                                            unsigned int* mipMapCount,
                                            unsigned int* faceCount,
                                            GLenum* faces)
{
    char PVRTCIdentifier[] = "PVR!";

    struct pvrtc_file_header_legacy
    {
        unsigned int size;         // size of the structure
        unsigned int height;       // height of surface to be created
        unsigned int width;        // width of input surface
        unsigned int mipmapCount;  // number of mip-map levels requested
        unsigned int formatflags;  // pixel format flags
        unsigned int dataSize;     // total size in bytes
        unsigned int bpp;          // number of bits per pixel
        unsigned int redBitMask;   // mask for red bit
        unsigned int greenBitMask; // mask for green bits
        unsigned int blueBitMask;  // mask for blue bits
        unsigned int alphaBitMask; // mask for alpha channel
        unsigned int pvrtcTag;     // magic number identifying pvrtc file
        unsigned int surfaceCount; // number of surfaces present in the pvrtc
    };

    // Read the file header.
    unsigned int size = sizeof(pvrtc_file_header_legacy);
    pvrtc_file_header_legacy header;
    unsigned int read = (int)stream->read(&header, 1, size);
    if (read != size)
    {
        GP_ERROR("Failed to read file header for pvrtc file '%s'.", path);
        return nullptr;
    }

    // Proper file header identifier.
    if (PVRTCIdentifier[0] != (char)((header.pvrtcTag >> 0) & 0xff)
        || PVRTCIdentifier[1] != (char)((header.pvrtcTag >> 8) & 0xff)
        || PVRTCIdentifier[2] != (char)((header.pvrtcTag >> 16) & 0xff)
        || PVRTCIdentifier[3] != (char)((header.pvrtcTag >> 24) & 0xff))
    {
        GP_ERROR("Failed to load pvrtc file '%s': invalid header.", path);
        return nullptr;
    }

    // Format flags for GLenum format.
    if (header.bpp == 4)
    {
        *format = header.alphaBitMask ? GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG
                                      : GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
    }
    else if (header.bpp == 2)
    {
        *format = header.alphaBitMask ? GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG
                                      : GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
    }
    else
    {
        GP_ERROR("Failed to load pvrtc file '%s': invalid pvrtc compressed texture format flags.",
                 path);
        return nullptr;
    }

    *width = (GLsizei)header.width;
    *height = (GLsizei)header.height;
    *mipMapCount = header.mipmapCount + 1; // +1 because mipmapCount does not include the base level
    *faceCount = 1;

    // Flags (needed legacy documentation on format, pre-PVR Format 3.0)
    if ((header.formatflags & 0x1000) != 0)
    {
        // Texture cube
        *faceCount = std::min(header.surfaceCount, 6u);
        for (size_t face = 0; face < (*faceCount); ++face)
        {
            faces[face] = GL_TEXTURE_CUBE_MAP_POSITIVE_X + face;
        }
    }
    else if ((header.formatflags & 0x4000) != 0)
    {
        // Volume texture
        GP_ERROR("Failed to load pvrtc file '%s': volume texture is not supported.", path);
        return nullptr;
    }

    unsigned int totalSize =
        header.dataSize; // Docs say dataSize is the size of the whole surface, or one face of a texture
                         // cube. But this does not appear to be the case with the latest PVRTexTool
    GLubyte* data = new GLubyte[totalSize];
    read = (int)stream->read(data, 1, totalSize);
    if (read != totalSize)
    {
        SAFE_DELETE_ARRAY(data);
        GP_ERROR("Failed to load texture data for pvrtc file '%s'.", path);
        return nullptr;
    }

    return data;
}

int Texture::getMaskByteIndex(unsigned int mask)
{
    switch (mask)
    {
        case 0xff000000:
            return 3;
        case 0x00ff0000:
            return 2;
        case 0x0000ff00:
            return 1;
        case 0x000000ff:
            return 0;
        default:
            return -1; // no or invalid mask
    }
}

Texture* Texture::createCompressedDDS(const std::string& path)
{
    // DDS file structures.
    struct dds_pixel_format
    {
        unsigned int dwSize;
        unsigned int dwFlags;
        unsigned int dwFourCC;
        unsigned int dwRGBBitCount;
        unsigned int dwRBitMask;
        unsigned int dwGBitMask;
        unsigned int dwBBitMask;
        unsigned int dwABitMask;
    };

    struct dds_header
    {
        unsigned int dwSize;
        unsigned int dwFlags;
        unsigned int dwHeight;
        unsigned int dwWidth;
        unsigned int dwPitchOrLinearSize;
        unsigned int dwDepth;
        unsigned int dwMipMapCount;
        unsigned int dwReserved1[11];
        dds_pixel_format ddspf;
        unsigned int dwCaps;
        unsigned int dwCaps2;
        unsigned int dwCaps3;
        unsigned int dwCaps4;
        unsigned int dwReserved2;
    };

    struct dds_mip_level
    {
        GLubyte* data;
        GLsizei width;
        GLsizei height;
        GLsizei size;
    };

    Texture* texture = nullptr;

    // Read DDS file.
    std::unique_ptr<Stream> stream(FileSystem::open(path));
    if (stream.get() == nullptr || !stream->canRead())
    {
        GP_ERROR("Failed to open file '%s'.", path);
        return nullptr;
    }

    // Validate DDS magic number.
    char code[4];
    if (stream->read(code, 1, 4) != 4 || strncmp(code, "DDS ", 4) != 0)
    {
        GP_ERROR("Failed to read DDS file '%s': invalid DDS magic number.", path);
        return nullptr;
    }

    // Read DDS header.
    dds_header header;
    if (stream->read(&header, sizeof(dds_header), 1) != 1)
    {
        GP_ERROR("Failed to read header for DDS file '%s'.", path);
        return nullptr;
    }

    if ((header.dwFlags & 0x20000 /*DDSD_MIPMAPCOUNT*/) == 0)
    {
        // Mipmap count not specified (non-mipmapped texture).
        header.dwMipMapCount = 1;
    }

    // Check type of images. Default is a regular texture
    unsigned int facecount = 1;
    GLenum faces[6] = { GL_TEXTURE_2D };
    GLenum target = GL_TEXTURE_2D;
    if ((header.dwCaps2 & 0x200 /*DDSCAPS2_CUBEMAP*/) != 0)
    {
        facecount = 0;
        for (size_t off = 0, flag = 0x400 /*DDSCAPS2_CUBEMAP_POSITIVEX*/; off < 6;
             ++off, flag <<= 1)
        {
            if ((header.dwCaps2 & flag) != 0)
            {
                faces[facecount++] = GL_TEXTURE_CUBE_MAP_POSITIVE_X + off;
            }
        }
        target = GL_TEXTURE_CUBE_MAP;
    }
    else if ((header.dwCaps2 & 0x200000 /*DDSCAPS2_VOLUME*/) != 0)
    {
        // Volume textures unsupported.
        GP_ERROR("Failed to create texture from DDS file '%s': volume textures are unsupported.",
                 path);
        return nullptr;
    }

    // Allocate mip level structures.
    dds_mip_level* mipLevels = new dds_mip_level[header.dwMipMapCount * facecount];
    memset(mipLevels, 0, sizeof(dds_mip_level) * header.dwMipMapCount * facecount);

    GLenum format = 0;
    GLenum internalFormat = 0;
    bool compressed = false;
    GLsizei width = header.dwWidth;
    GLsizei height = header.dwHeight;
    Texture::Format textureFormat = Texture::UNKNOWN;

    if (header.ddspf.dwFlags & 0x4 /*DDPF_FOURCC*/)
    {
        compressed = true;
        int bytesPerBlock;

        // Compressed.
        switch (header.ddspf.dwFourCC)
        {
            case ('D' | ('X' << 8) | ('T' << 16) | ('1' << 24)):
                format = internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
                bytesPerBlock = 8;
                break;
            case ('D' | ('X' << 8) | ('T' << 16) | ('3' << 24)):
                format = internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
                bytesPerBlock = 16;
                break;
            case ('D' | ('X' << 8) | ('T' << 16) | ('5' << 24)):
                format = internalFormat = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
                bytesPerBlock = 16;
                break;
            case ('A' | ('T' << 8) | ('C' << 16) | (' ' << 24)):
                format = internalFormat = ATC_RGB_AMD;
                bytesPerBlock = 8;
                break;
            case ('A' | ('T' << 8) | ('C' << 16) | ('A' << 24)):
                format = internalFormat = ATC_RGBA_EXPLICIT_ALPHA_AMD;
                bytesPerBlock = 16;
                break;
            case ('A' | ('T' << 8) | ('C' << 16) | ('I' << 24)):
                format = internalFormat = ATC_RGBA_INTERPOLATED_ALPHA_AMD;
                bytesPerBlock = 16;
                break;
            case ('E' | ('T' << 8) | ('C' << 16) | ('1' << 24)):
                format = internalFormat = ETC1_RGB8;
                bytesPerBlock = 8;
                break;
            default:
                GP_ERROR("Unsupported compressed texture format (%d) for DDS file '%s'.",
                         header.ddspf.dwFourCC,
                         path);
                SAFE_DELETE_ARRAY(mipLevels);
                return nullptr;
        }

        for (size_t face = 0; face < facecount; ++face)
        {
            for (size_t i = 0; i < header.dwMipMapCount; ++i)
            {
                dds_mip_level& level = mipLevels[i + face * header.dwMipMapCount];

                level.width = width;
                level.height = height;
                level.size =
                    std::max(1, (width + 3) >> 2) * std::max(1, (height + 3) >> 2) * bytesPerBlock;
                level.data = new GLubyte[level.size];

                if (stream->read(level.data, 1, level.size) != (unsigned int)level.size)
                {
                    GP_ERROR("Failed to load dds compressed texture bytes for texture: %s", path);

                    // Cleanup mip data.
                    for (size_t face = 0; face < facecount; ++face)
                        for (size_t i = 0; i < header.dwMipMapCount; ++i)
                            SAFE_DELETE_ARRAY(mipLevels[i + face * header.dwMipMapCount].data);
                    SAFE_DELETE_ARRAY(mipLevels);
                    return texture;
                }

                width = std::max(1, width >> 1);
                height = std::max(1, height >> 1);
            }
            width = header.dwWidth;
            height = header.dwHeight;
        }
    }
    else if (header.ddspf.dwFlags & 0x40 /*DDPF_RGB*/)
    {
        // RGB/RGBA (uncompressed)
        bool colorConvert = false;
        unsigned int rmask = header.ddspf.dwRBitMask;
        unsigned int gmask = header.ddspf.dwGBitMask;
        unsigned int bmask = header.ddspf.dwBBitMask;
        unsigned int amask = header.ddspf.dwABitMask;
        int ridx = getMaskByteIndex(rmask);
        int gidx = getMaskByteIndex(gmask);
        int bidx = getMaskByteIndex(bmask);
        int aidx = getMaskByteIndex(amask);

        if (header.ddspf.dwRGBBitCount == 24)
        {
            format = internalFormat = GL_RGB;
            textureFormat = Texture::RGB;
            colorConvert = (ridx != 0) || (gidx != 1) || (bidx != 2);
        }
        else if (header.ddspf.dwRGBBitCount == 32)
        {
            format = internalFormat = GL_RGBA;
            textureFormat = Texture::RGBA;
            if (ridx == 0 && gidx == 1 && bidx == 2)
            {
                aidx = 3; // XBGR or ABGR
                colorConvert = false;
            }
            else if (ridx == 2 && gidx == 1 && bidx == 0)
            {
                aidx = 3; // XRGB or ARGB
                colorConvert = true;
            }
            else
            {
                format = 0; // invalid format
            }
        }

        if (format == 0)
        {
            GP_ERROR("Failed to create texture from uncompressed DDS file '%s': Unsupported color "
                     "format (must be one of R8G8B8, A8R8G8B8, A8B8G8R8, X8R8G8B8, X8B8G8R8.",
                     path);
            SAFE_DELETE_ARRAY(mipLevels);
            return nullptr;
        }

        // Read data.
        for (size_t face = 0; face < facecount; ++face)
        {
            for (size_t i = 0; i < header.dwMipMapCount; ++i)
            {
                dds_mip_level& level = mipLevels[i + face * header.dwMipMapCount];

                level.width = width;
                level.height = height;
                level.size = width * height * (header.ddspf.dwRGBBitCount >> 3);
                level.data = new GLubyte[level.size];

                if (stream->read(level.data, 1, level.size) != (unsigned int)level.size)
                {
                    GP_ERROR("Failed to load bytes for RGB dds texture: %s", path);

                    // Cleanup mip data.
                    for (size_t face = 0; face < facecount; ++face)
                        for (size_t i = 0; i < header.dwMipMapCount; ++i)
                            SAFE_DELETE_ARRAY(mipLevels[i + face * header.dwMipMapCount].data);
                    SAFE_DELETE_ARRAY(mipLevels);
                    return texture;
                }

                width = std::max(1, width >> 1);
                height = std::max(1, height >> 1);
            }
            width = header.dwWidth;
            height = header.dwHeight;
        }

        // Perform color conversion.
        if (colorConvert)
        {
            // Note: While it's possible to use BGRA_EXT texture formats here and avoid CPU color
            // conversion below, there seems to be different flavors of the BGRA extension, with
            // some vendors requiring an internal format of RGBA and others requiring an internal
            // format of BGRA. We could be smarter here later and skip color conversion in favor of
            // GL_BGRA_EXT (for format and/or internal format) based on which GL extensions are
            // available. Tip: Using A8B8G8R8 and X8B8G8R8 DDS format maps directly to GL RGBA and
            // requires on no color conversion.
            GLubyte *pixel, r, g, b, a;
            if (format == GL_RGB)
            {
                for (size_t face = 0; face < facecount; ++face)
                {
                    for (size_t i = 0; i < header.dwMipMapCount; ++i)
                    {
                        dds_mip_level& level = mipLevels[i + face * header.dwMipMapCount];
                        for (size_t j = 0; j < level.size; j += 3)
                        {
                            pixel = &level.data[j];
                            r = pixel[ridx];
                            g = pixel[gidx];
                            b = pixel[bidx];
                            pixel[0] = r;
                            pixel[1] = g;
                            pixel[2] = b;
                        }
                    }
                }
            }
            else if (format == GL_RGBA)
            {
                for (size_t face = 0; face < facecount; ++face)
                {
                    for (size_t i = 0; i < header.dwMipMapCount; ++i)
                    {
                        dds_mip_level& level = mipLevels[i + face * header.dwMipMapCount];
                        for (size_t j = 0; j < level.size; j += 4)
                        {
                            pixel = &level.data[j];
                            r = pixel[ridx];
                            g = pixel[gidx];
                            b = pixel[bidx];
                            a = pixel[aidx];
                            pixel[0] = r;
                            pixel[1] = g;
                            pixel[2] = b;
                            pixel[3] = a;
                        }
                    }
                }
            }
        }
    }
    else
    {
        // Unsupported.
        GP_ERROR("Failed to create texture from DDS file '%s': unsupported flags (%d).",
                 path,
                 header.ddspf.dwFlags);
        SAFE_DELETE_ARRAY(mipLevels);
        return nullptr;
    }

    // Close file.
    stream->close();

    // Generate GL texture.
    GLuint textureId;
    GL_ASSERT(glGenTextures(1, &textureId));
    GL_ASSERT(glBindTexture(target, textureId));

    Filter minFilter = header.dwMipMapCount > 1 ? NEAREST_MIPMAP_LINEAR : LINEAR;
    GL_ASSERT(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, minFilter));

    // Create gameplay texture.
    texture = new Texture();
    texture->_handle = textureId;
    texture->_type = (Type)target;
    texture->_width = header.dwWidth;
    texture->_height = header.dwHeight;
    texture->_compressed = compressed;
    texture->_mipmapped = header.dwMipMapCount > 1;
    texture->_minFilter = minFilter;

    // Load texture data.
    for (size_t face = 0; face < facecount; ++face)
    {
        GLenum texImageTarget = faces[face];
        for (size_t i = 0; i < header.dwMipMapCount; ++i)
        {
            dds_mip_level& level = mipLevels[i + face * header.dwMipMapCount];
            if (compressed)
            {
                GL_ASSERT(glCompressedTexImage2D(texImageTarget,
                                                 i,
                                                 format,
                                                 level.width,
                                                 level.height,
                                                 0,
                                                 level.size,
                                                 level.data));
            }
            else
            {
                GL_ASSERT(glTexImage2D(texImageTarget,
                                       i,
                                       internalFormat,
                                       level.width,
                                       level.height,
                                       0,
                                       format,
                                       GL_UNSIGNED_BYTE,
                                       level.data));
            }

            // Clean up the texture data.
            SAFE_DELETE_ARRAY(level.data);
        }
    }

    // Clean up mip levels structure.
    SAFE_DELETE_ARRAY(mipLevels);

    // Restore the texture id
    GL_ASSERT(glBindTexture((GLenum)__currentTextureType, __currentTextureId));

    return texture;
}

void Texture::generateMipmaps()
{
    if (!_mipmapped)
    {
        GLenum target = (GLenum)_type;
        GL_ASSERT(glBindTexture(target, _handle));
        GL_ASSERT(glHint(GL_GENERATE_MIPMAP_HINT, GL_NICEST));
        if (std::addressof(glGenerateMipmap)) GL_ASSERT(glGenerateMipmap(target));

        _mipmapped = true;

        // Restore the texture id
        GL_ASSERT(glBindTexture((GLenum)__currentTextureType, __currentTextureId));
    }
}

Texture::Sampler::Sampler(Texture* texture)
    : _texture(texture), _wrapS(Texture::REPEAT), _wrapT(Texture::REPEAT), _wrapR(Texture::REPEAT)
{
    assert(texture);
    _minFilter = texture->_minFilter;
    _magFilter = texture->_magFilter;
}

Texture::Sampler::~Sampler() { SAFE_RELEASE(_texture); }

Texture::Sampler* Texture::Sampler::create(Texture* texture)
{
    assert(texture);
    assert(texture->_type == Texture::TEXTURE_2D || texture->_type == Texture::TEXTURE_CUBE);
    texture->addRef();
    return new Sampler(texture);
}

Texture::Sampler* Texture::Sampler::create(const std::string& path, bool generateMipmaps)
{
    Texture* texture = Texture::create(path, generateMipmaps);
    return texture ? new Sampler(texture) : nullptr;
}

void Texture::Sampler::setWrapMode(Wrap wrapS, Wrap wrapT, Wrap wrapR)
{
    _wrapS = wrapS;
    _wrapT = wrapT;
    _wrapR = wrapR;
}

void Texture::Sampler::setFilterMode(Filter minificationFilter, Filter magnificationFilter)
{
    _minFilter = minificationFilter;
    _magFilter = magnificationFilter;
}

void Texture::Sampler::bind()
{
    assert(_texture);

    GLenum target = (GLenum)_texture->_type;
    if (__currentTextureId != _texture->_handle)
    {
        GL_ASSERT(glBindTexture(target, _texture->_handle));
        __currentTextureId = _texture->_handle;
        __currentTextureType = _texture->_type;
    }

    if (_texture->_minFilter != _minFilter)
    {
        _texture->_minFilter = _minFilter;
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_MIN_FILTER, (GLenum)_minFilter));
    }

    if (_texture->_magFilter != _magFilter)
    {
        _texture->_magFilter = _magFilter;
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_MAG_FILTER, (GLenum)_magFilter));
    }

    if (_texture->_wrapS != _wrapS)
    {
        _texture->_wrapS = _wrapS;
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_WRAP_S, (GLenum)_wrapS));
    }

    if (_texture->_wrapT != _wrapT)
    {
        _texture->_wrapT = _wrapT;
        GL_ASSERT(glTexParameteri(target, GL_TEXTURE_WRAP_T, (GLenum)_wrapT));
    }

#if defined(GL_TEXTURE_WRAP_R) // OpenGL ES 3.x and up, OpenGL 1.2 and up
    if (_texture->_wrapR != _wrapR)
    {
        _texture->_wrapR = _wrapR;
        if (target
            == GL_TEXTURE_CUBE_MAP) // We don't want to run this on something that we know will fail
            GL_ASSERT(glTexParameteri(target, GL_TEXTURE_WRAP_R, (GLenum)_wrapR));
    }
#endif
}

} // namespace tractor
