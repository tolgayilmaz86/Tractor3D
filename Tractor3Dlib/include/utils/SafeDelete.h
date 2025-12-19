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

#include <memory>
#include <type_traits>

namespace tractor
{

/**
 * @brief Modern C++ utilities for safe pointer and resource cleanup.
 * 
 * These functions provide type-safe alternatives to raw delete operations.
 * 
 * IMPORTANT: These functions require that the destructor is accessible (public).
 * For classes with private/protected destructors (common in the Ref pattern),
 * use the SAFE_DELETE macro which expands inline and can access destructors
 * in friend/member contexts, or use tractor::safeRelease() for Ref-derived objects.
 * 
 * Migration guide:
 *   - For Ref-derived objects: use tractor::safeRelease(x) or SAFE_RELEASE(x)
 *   - For objects with public destructors: use tractor::safeDelete(x)
 *   - For arrays: use tractor::safeDeleteArray(x)
 *   - For smart pointers: they clean up automatically, or use .reset()
 */

/**
 * @brief Safely deletes a pointer and sets it to nullptr.
 * 
 * @tparam T Pointer type (deduced automatically)
 * @param ptr Reference to the pointer to delete
 * 
 * Example:
 *   MyClass* obj = new MyClass();
 *   tractor::safeDelete(obj);  // obj is now nullptr
 */
template <typename T>
inline void safeDelete(T*& ptr) noexcept
{
    delete ptr;
    ptr = nullptr;
}

/**
 * @brief Safely deletes an array pointer and sets it to nullptr.
 * 
 * @tparam T Array element type (deduced automatically)
 * @param ptr Reference to the array pointer to delete
 * 
 * Example:
 *   char* buffer = new char[1024];
 *   tractor::safeDeleteArray(buffer);  // buffer is now nullptr
 */
template <typename T>
inline void safeDeleteArray(T*& ptr) noexcept
{
    delete[] ptr;
    ptr = nullptr;
}

/**
 * @brief Overload for const pointers - deletes and sets to nullptr.
 * 
 * @tparam T Pointer type (deduced automatically)
 * @param ptr Reference to the const pointer to delete
 */
template <typename T>
inline void safeDelete(const T*& ptr) noexcept
{
    delete ptr;
    ptr = nullptr;
}

/**
 * @brief Overload for const array pointers - deletes and sets to nullptr.
 * 
 * @tparam T Array element type (deduced automatically)
 * @param ptr Reference to the const array pointer to delete
 */
template <typename T>
inline void safeDeleteArray(const T*& ptr) noexcept
{
    delete[] ptr;
    ptr = nullptr;
}

/**
 * @brief No-op overload for nullptr_t to handle edge cases gracefully.
 */
inline void safeDelete(std::nullptr_t) noexcept {}

/**
 * @brief No-op overload for nullptr_t to handle edge cases gracefully.
 */
inline void safeDeleteArray(std::nullptr_t) noexcept {}

/**
 * @brief Safely resets a unique_ptr.
 * 
 * @tparam T Managed type
 * @tparam Deleter Deleter type
 * @param ptr Reference to the unique_ptr to reset
 */
template <typename T, typename Deleter>
inline void safeDelete(std::unique_ptr<T, Deleter>& ptr) noexcept
{
    ptr.reset();
}

/**
 * @brief Safely resets a shared_ptr.
 * 
 * @tparam T Managed type
 * @param ptr Reference to the shared_ptr to reset
 */
template <typename T>
inline void safeDelete(std::shared_ptr<T>& ptr) noexcept
{
    ptr.reset();
}

} // namespace tractor
