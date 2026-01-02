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

namespace tractor
{

/**
 * @defgroup ClonePatterns Clone Patterns
 * @brief Consistent cloning patterns used throughout the engine.
 * 
 * The engine uses several clone patterns for different scenarios:
 * 
 * ## Simple Cloning (Cloneable<T>)
 * For objects that can be cloned without additional context:
 * @code
 * class MyClass : public Cloneable<MyClass> {
 * public:
 *     std::shared_ptr<MyClass> clone() const override {
 *         return std::make_shared<MyClass>(*this);
 *     }
 * };
 * @endcode
 * 
 * ## Context-Aware Cloning (CloneableWith<T, Context>)
 * For objects that need context during cloning (e.g., scene graph objects):
 * @code
 * class Material : public CloneableWith<Material, NodeCloneContext> {
 * public:
 *     std::shared_ptr<Material> clone(NodeCloneContext& context) const override;
 * };
 * @endcode
 * 
 * ## Drawable Cloning
 * Drawable objects use the cloneDrawable() pattern for consistent shared_ptr semantics:
 * @code
 * class MyDrawable : public Drawable {
 * public:
 *     DrawablePtr cloneDrawable(NodeCloneContext& context) const override {
 *         return std::make_shared<MyDrawable>(*this);
 *     }
 * };
 * @endcode
 * 
 * ## Node Component Cloning  
 * Components like Camera, Light, AudioSource use clone(NodeCloneContext&) returning shared_ptr:
 * @code
 * CameraPtr Camera::clone(NodeCloneContext& context);
 * LightPtr Light::clone(NodeCloneContext& context);
 * AudioSourcePtr AudioSource::clone(NodeCloneContext& context);
 * @endcode
 * @{
 */

/**
 * Interface for objects that support simple cloning (Prototype Pattern).
 * 
 * This interface provides a standardized way to clone objects in the engine.
 * Classes implementing this interface should return a deep copy of themselves.
 * 
 * @tparam T The type of object being cloned (typically the implementing class itself)
 * 
 * Usage example:
 * @code
 * class MyClass : public Cloneable<MyClass>, public std::enable_shared_from_this<MyClass> {
 * public:
 *     std::shared_ptr<MyClass> clone() const override {
 *         return std::make_shared<MyClass>(*this);
 *     }
 * };
 * @endcode
 */
template<typename T>
class Cloneable
{
public:
    using Ptr = std::shared_ptr<T>;

    virtual ~Cloneable() = default;

    /**
     * Creates a deep copy of this object.
     * 
     * @return A new instance that is a copy of this object.
     */
    virtual Ptr clone() const = 0;
};

/**
 * Interface for objects that support cloning with a custom context parameter.
 * 
 * This interface is used when cloning requires additional context, such as when
 * cloning scene nodes where references to other cloned nodes need to be tracked.
 * 
 * @tparam T The type of object being cloned
 * @tparam Context The context type required for cloning (e.g., NodeCloneContext)
 * 
 * Usage example:
 * @code
 * class Material : public CloneableWith<Material, NodeCloneContext> {
 * public:
 *     std::shared_ptr<Material> clone(NodeCloneContext& context) const override {
 *         auto copy = std::make_shared<Material>();
 *         // Clone internal state using context...
 *         return copy;
 *     }
 * };
 * @endcode
 */
template<typename T, typename Context>
class CloneableWith
{
public:
    using Ptr = std::shared_ptr<T>;

    virtual ~CloneableWith() = default;

    /**
     * Creates a deep copy of this object using the provided context.
     * 
     * @param context The cloning context for tracking cloned objects.
     * @return A new instance that is a copy of this object.
     */
    virtual Ptr clone(Context& context) const = 0;
};

/** @} */ // end of ClonePatterns group

} // namespace tractor
