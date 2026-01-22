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
 * @defgroup FactoryPatterns Factory Patterns
 * @brief Consistent object creation patterns used throughout the engine.
 * 
 * The engine uses factory patterns for consistent object creation with proper
 * lifetime management via shared_ptr.
 * 
 * ## Available Patterns
 * 
 * ### 1. CreatableWithKey<T> (Recommended)
 * Uses passkey idiom - public constructor but restricted instantiation:
 * @code
 * class MyClass : public CreatableWithKey<MyClass> {
 * public:
 *     explicit MyClass(FactoryKey, int value) : _value(value) {}
 * private:
 *     int _value;
 * };
 * // auto obj = MyClass::create(42);
 * @endcode
 * 
 * ### 2. StandaloneCreatableWithKey<T>
 * Same as above but without enable_shared_from_this:
 * @code
 * class MyUtil : public StandaloneCreatableWithKey<MyUtil> {
 * public:
 *     explicit MyUtil(FactoryKey) {}
 * };
 * @endcode
 * 
 * @{
 */

/**
 * Passkey class used to restrict constructor access to factory methods.
 * 
 * This is a separate class (not nested) to allow it to be used with std::make_shared.
 */
class FactoryKey
{
    template<typename T> friend class CreatableWithKey;
    template<typename T> friend class StandaloneCreatableWithKey;
    
    FactoryKey() = default;
public:
    // Copy/move must be public for std::make_shared to work
    FactoryKey(const FactoryKey&) = default;
    FactoryKey(FactoryKey&&) = default;
    FactoryKey& operator=(const FactoryKey&) = default;
    FactoryKey& operator=(FactoryKey&&) = default;
};

/**
 * CRTP base class using the passkey idiom for factory pattern with enable_shared_from_this.
 * 
 * The derived class must have a public constructor that takes FactoryKey as the first parameter.
 * This ensures objects can only be created via the create() factory method.
 * 
 * @tparam T The derived class type
 * 
 * Usage:
 * @code
 * class MyClass : public CreatableWithKey<MyClass> {
 * public:
 *     explicit MyClass(FactoryKey, int value) : _value(value) {}
 *     int getValue() const { return _value; }
 * private:
 *     int _value{0};
 * };
 * 
 * // Usage:
 * auto obj = MyClass::create(42);  // Works
 * // MyClass m(42);  // Error - no matching constructor without FactoryKey
 * @endcode
 */
template<typename T>
class CreatableWithKey : public std::enable_shared_from_this<T>
{
public:
    using Ptr = std::shared_ptr<T>;
    using WeakPtr = std::weak_ptr<T>;

    virtual ~CreatableWithKey() = default;

    /**
     * Factory method that creates a new instance of T.
     * 
     * @tparam Args Constructor argument types (after the FactoryKey)
     * @param args Arguments to forward to T's constructor
     * @return A shared_ptr to the newly created instance
     */
    template<typename... Args>
    static Ptr create(Args&&... args)
    {
        return std::make_shared<T>(FactoryKey{}, std::forward<Args>(args)...);
    }

protected:
    CreatableWithKey() = default;
    CreatableWithKey(const CreatableWithKey&) = default;
    CreatableWithKey(CreatableWithKey&&) = default;
    CreatableWithKey& operator=(const CreatableWithKey&) = default;
    CreatableWithKey& operator=(CreatableWithKey&&) = default;
};

/**
 * Standalone version of passkey factory (without enable_shared_from_this).
 * 
 * Use this for classes that don't need shared_from_this().
 * 
 * @tparam T The derived class type
 */
template<typename T>
class StandaloneCreatableWithKey
{
public:
    using Ptr = std::shared_ptr<T>;
    using WeakPtr = std::weak_ptr<T>;

    virtual ~StandaloneCreatableWithKey() = default;

    template<typename... Args>
    static Ptr create(Args&&... args)
    {
        return std::make_shared<T>(FactoryKey{}, std::forward<Args>(args)...);
    }

protected:
    StandaloneCreatableWithKey() = default;
    StandaloneCreatableWithKey(const StandaloneCreatableWithKey&) = default;
    StandaloneCreatableWithKey(StandaloneCreatableWithKey&&) = default;
    StandaloneCreatableWithKey& operator=(const StandaloneCreatableWithKey&) = default;
    StandaloneCreatableWithKey& operator=(StandaloneCreatableWithKey&&) = default;
};

/**
 * Helper macro to declare standard pointer type aliases for a class.
 * 
 * @param ClassName The name of the class
 */
#define TRACTOR_DECLARE_PTR_TYPES(ClassName) \
    using ClassName##Ptr = std::shared_ptr<ClassName>; \
    using ClassName##WeakPtr = std::weak_ptr<ClassName>;

/** @} */ // end of FactoryPatterns group

} // namespace tractor
