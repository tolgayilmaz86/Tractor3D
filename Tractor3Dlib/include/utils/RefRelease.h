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

namespace detail
{
    // SFINAE helper to detect if type has release() method
    template <typename T, typename = void>
    struct has_release : std::false_type {};

    template <typename T>
    struct has_release<T, std::void_t<decltype(std::declval<T>().release())>> : std::true_type {};

    template <typename T>
    inline constexpr bool has_release_v = has_release<T>::value;
}

/**
 * @brief Safely releases a Ref-derived object and sets the pointer to nullptr.
 * 
 * This function is for legacy intrusive reference-counted objects that inherit
 * from tractor::Ref and have addRef()/release() methods.
 * 
 * For types without release(), this function will delete the object instead.
 * 
 * @tparam T Type that may have a release() method (e.g., Ref-derived classes)
 * @param p Reference to the pointer to release
 * 
 * Example:
 *   Scene* scene = Scene::create();
 *   tractor::safeRelease(scene);  // scene is now nullptr
 */
template <typename T>
inline void safeRelease(T*& p)
{
    if (p)
    {
        if constexpr (detail::has_release_v<T>)
        {
            p->release();
        }
        else
        {
            delete p;
        }
        p = nullptr;
    }
}

/**
 * @brief Safely resets a shared_ptr.
 * 
 * @tparam T Managed type
 * @param p Reference to the shared_ptr to reset
 */
template <typename T>
inline void safeRelease(std::shared_ptr<T>& p)
{
    p.reset();
}

/**
 * @brief Safely resets a weak_ptr.
 * 
 * @tparam T Managed type
 * @param p Reference to the weak_ptr to reset
 */
template <typename T>
inline void safeRelease(std::weak_ptr<T>& p)
{
    p.reset();
}

/**
 * @brief No-op overload for nullptr_t to handle edge cases gracefully.
 */
inline void safeRelease(std::nullptr_t) noexcept {}

// =============================================================================
// DEPRECATED NAMESPACE - Maintained for backward compatibility
// =============================================================================
// The tractor::ref::safeRelease functions are deprecated.
// Use tractor::safeRelease directly instead.
// =============================================================================
namespace ref
{
    // Intrusive pointer (old pattern): calls release() and nulls pointer
    template <typename T>
    [[deprecated("Use tractor::safeRelease instead of tractor::ref::safeRelease")]]
    inline void safeRelease(T*& p)
    {
        tractor::safeRelease(p);
    }

    // shared_ptr overload: reset
    template <typename T>
    [[deprecated("Use tractor::safeRelease instead of tractor::ref::safeRelease")]]
    inline void safeRelease(std::shared_ptr<T>& p)
    {
        tractor::safeRelease(p);
    }

    // weak_ptr overload: reset
    template <typename T>
    [[deprecated("Use tractor::safeRelease instead of tractor::ref::safeRelease")]]
    inline void safeRelease(std::weak_ptr<T>& p)
    {
        tractor::safeRelease(p);
    }

} // namespace ref

} // namespace tractor
