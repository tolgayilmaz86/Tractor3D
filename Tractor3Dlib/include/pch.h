#pragma once

// C/C++
#include <new>
#include <memory>
#include <cstdio>
#include <cstdlib>
#include <cassert>
#include <cwchar>
#include <cwctype>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <ctime>
#include <iostream>
#include <sstream>
#include <string>
#include <cstring>
#include <vector>
#include <ranges>
#include <algorithm>
#include <list>
#include <set>
#include <stack>
#include <map>
#include <unordered_map>
#include <queue>
#include <limits>
#include <functional>
#include <bitset>
#include <typeinfo>
#include <thread>
#include <mutex>
#include <chrono>
#include "utils/Logger.h"

// Bring common functions from C into global namespace
using std::memcpy;
using std::fabs;
using std::sqrt;
using std::cos;
using std::sin;
using std::tan;
using std::isspace;
using std::isdigit;
using std::toupper;
using std::tolower;
using std::size_t;
using std::min;
using std::max;
using std::modf;
using std::atoi;

constexpr auto EMPTY_STRING{ "" };

namespace tractor
{
    /**
     * Print logging (implemented per platform).
     * @script{ignore}
     */
    extern void print(const char* format, ...);
}

// Current function macro.
#define __current__func__ __FUNCTION__
#define DEBUG_BREAK() __debugbreak()

// Error macro.
#ifdef GP_ERRORS_AS_WARNINGS
#define GP_ERROR GP_WARN
#else
#define GP_ERROR(...) do \
    { \
        tractor::Logger::log(tractor::Logger::LEVEL_ERROR, "%s -- ", __current__func__); \
        tractor::Logger::log(tractor::Logger::LEVEL_ERROR, __VA_ARGS__); \
        tractor::Logger::log(tractor::Logger::LEVEL_ERROR, "\n"); \
        DEBUG_BREAK(); \
        assert(0); \
        std::exit(-1); \
    } while (0)
#endif

// Warning macro.
#define GP_WARN(...) do \
    { \
        tractor::Logger::log(tractor::Logger::LEVEL_WARN, "%s -- ", __current__func__); \
        tractor::Logger::log(tractor::Logger::LEVEL_WARN, __VA_ARGS__); \
        tractor::Logger::log(tractor::Logger::LEVEL_WARN, "\n"); \
    } while (0)

#pragma warning( disable : 4005 )
#pragma warning( disable : 4172 )
#pragma warning( disable : 4244 )
#pragma warning( disable : 4267 )
#pragma warning( disable : 4311 )
#pragma warning( disable : 4316 )
#pragma warning( disable : 4390 )
#pragma warning( disable : 4800 )
#pragma warning( disable : 4996 )

// Bullet Physics
#include <btBulletDynamicsCommon.h>
#include <btBulletCollisionCommon.h>

#define BV(v) (btVector3((v).x, (v).y, (v).z))
#define BQ(q) (btQuaternion((q).x, (q).y, (q).z, (q).w))

// Debug new for memory leak detection
#include "utils/DebugNew.h"
#include "utils/Ref.h"

// Object deletion macro
#define SAFE_DELETE(x) \
    { \
        delete x; \
        x = nullptr; \
    }

// Array deletion macro
#define SAFE_DELETE_ARRAY(x) \
    { \
        delete[] x; \
        x = nullptr; \
    }

// Ref cleanup macro
#define SAFE_RELEASE(x) \
    if (x) \
    { \
        (x)->release(); \
        x = nullptr; \
    }

// Math
#define MATH_DEG_TO_RAD(x)          ((x) * 0.0174532925f)
#define MATH_RAD_TO_DEG(x)          ((x)* 57.29577951f)
#define MATH_RANDOM_MINUS1_1()      ((2.0f*((float)rand()/RAND_MAX))-1.0f)      // Returns a random float between -1 and 1.
#define MATH_RANDOM_0_1()           ((float)rand()/RAND_MAX)                    // Returns a random float between 0 and 1.
#define MATH_CLAMP(x, lo, hi)       ((x < lo) ? lo : ((x > hi) ? hi : x))

constexpr auto MATH_FLOAT_SMALL = 1.0e-37f;
constexpr auto MATH_TOLERANCE   = 2e-37f;
constexpr auto MATH_E           = 2.71828182845904523536f;
constexpr auto MATH_LOG10E      = 0.4342944819032518f;
constexpr auto MATH_LOG2E       = 1.442695040888963387f;
constexpr auto MATH_PI          = 3.14159265358979323846f;
constexpr auto MATH_PIOVER2     = 1.57079632679489661923f;
constexpr auto MATH_PIOVER4     = 0.785398163397448309616f;
constexpr auto MATH_PIX2        = 6.28318530717958647693f;
constexpr auto MATH_EPSILON     = 0.000001f;

#ifndef M_1_PI
constexpr auto M_1_PI           = 0.31830988618379067154;
#endif

// NOMINMAX makes sure that windef.h doesn't add macros min and max
#define NOMINMAX

#define AL_LIBTYPE_STATIC
#include <AL/al.h>
#include <AL/alc.h>

// Compressed Media
#include <vorbis/vorbisfile.h>

// Image
#include <png.h>

// Scripting
using std::va_list;
#include <lua.hpp>

#define WINDOW_VSYNC        1

#define WIN32_LEAN_AND_MEAN
#include <GL/glew.h>
#include <GL/gl.h>
#define GP_USE_VAO

// Graphics (GLSL)
constexpr auto VERTEX_ATTRIBUTE_POSITION_NAME        = "a_position";
constexpr auto VERTEX_ATTRIBUTE_NORMAL_NAME          = "a_normal";
constexpr auto VERTEX_ATTRIBUTE_COLOR_NAME           = "a_color";
constexpr auto VERTEX_ATTRIBUTE_TANGENT_NAME         = "a_tangent";
constexpr auto VERTEX_ATTRIBUTE_BINORMAL_NAME        = "a_binormal";
constexpr auto VERTEX_ATTRIBUTE_BLENDWEIGHTS_NAME    = "a_blendWeights";
constexpr auto VERTEX_ATTRIBUTE_BLENDINDICES_NAME    = "a_blendIndices";
constexpr auto VERTEX_ATTRIBUTE_TEXCOORD_PREFIX_NAME = "a_texCoord";

// Hardware buffer
namespace tractor
{
/** Vertex attribute. */
using VertexAttribute = GLint;
/** Vertex buffer handle. */
using VertexBufferHandle = GLuint;
/** Index buffer handle. */
using IndexBufferHandle = GLuint;
/** Texture handle. */
using TextureHandle = GLuint;
/** Frame buffer handle. */
using FrameBufferHandle = GLuint;
/** Render buffer handle. */
using RenderBufferHandle = GLuint;
/** Gamepad handle */
using GamepadHandle = unsigned long;
} // namespace tractor

/**
 * GL assertion that can be used for any OpenGL function call.
 *
 * This macro will assert if an error is detected when executing
 * the specified GL code. This macro will do nothing in release
 * mode and is therefore safe to use for realtime/per-frame GL
 * function calls.
 */
#if defined(NDEBUG)
#define GL_ASSERT( gl_code ) gl_code
#else
#define GL_ASSERT( gl_code ) do \
    { \
        gl_code; \
        __gl_error_code = glGetError(); \
        assert(__gl_error_code == GL_NO_ERROR); \
    } while(0)
#endif

 /** Global variable to hold GL errors
  * @script{ignore} */
extern GLenum __gl_error_code;

/**
 * Executes the specified AL code and checks the AL error afterwards
 * to ensure it succeeded.
 *
 * The AL_LAST_ERROR macro can be used afterwards to check whether a AL error was
 * encountered executing the specified code.
 */
#define AL_CHECK( al_code ) do \
    { \
        while (alGetError() != AL_NO_ERROR) ; \
        al_code; \
        __al_error_code = alGetError(); \
        if (__al_error_code != AL_NO_ERROR) \
        { \
            GP_ERROR(#al_code ": %d", (int)__al_error_code); \
        } \
    } while(0)

 /** Global variable to hold AL errors
  * @script{ignore} */
extern ALenum __al_error_code;

/**
 * Accesses the most recently set global AL error.
 */
#define AL_LAST_ERROR() __al_error_code

#define DEFINE_ITERATOR(StructType)                             \
    class Iterator {                                            \
    public:                                                     \
        StructType* current;                                    \
                                                                \
        explicit Iterator(StructType* entry) : current(entry) {}\
                                                                \
        StructType& operator*() { return *current; }            \
        StructType* operator->() { return current; }            \
                                                                \
        Iterator& operator++() {                                \
            if (current) current = current->next;               \
            return *this;                                       \
        }                                                       \
                                                                \
        Iterator operator++(int) {                              \
            Iterator temp = *this;                              \
            ++(*this);                                          \
            return temp;                                        \
        }                                                       \
                                                                \
        bool operator!=(const Iterator& other) const {          \
            return current != other.current;                    \
        }                                                       \
    };                                                          \
                                                                \
    Iterator begin() { return Iterator(this); }                 \
    Iterator end() { return Iterator(nullptr); }
