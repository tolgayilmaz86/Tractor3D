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

class Node;
class NodeCloneContext;
class Drawable;

/** Shared pointer type for Drawable. */
using DrawablePtr = std::shared_ptr<Drawable>;

/** Weak pointer type for Drawable. */
using DrawableWeakPtr = std::weak_ptr<Drawable>;

/**
 * Defines a drawable object that can be attached to a Node.
 * 
 * Drawable classes support cloning via the clone() method which returns
 * a shared_ptr for consistent lifetime management across the engine.
 */
class Drawable
{
    friend class Node;

  public:
    /**
     * Enumeration of drawable types for fast type identification.
     * Use getType() instead of dynamic_cast for better performance.
     */
    enum class Type
    {
        MODEL,
        SPRITE,
        PARTICLE_EMITTER,
        TERRAIN,
        TILESET,
        TEXT,
        FORM
    };

    /**
     * Constructor.
     */
    Drawable() = default;

    /**
     * Destructor.
     */
    virtual ~Drawable() = default;

    /**
     * Returns the type of this drawable.
     * Use this instead of dynamic_cast for type checking.
     *
     * @return The drawable type.
     */
    virtual Type getType() const noexcept = 0;

    /**
     * Draws the object.
     *
     * @param wireframe true if you want to request to draw the wireframe only.
     * @return The number of graphics draw calls required to draw the object.
     */
    virtual unsigned int draw(bool wireframe = false) = 0;

    /**
     * Gets the node this drawable is attached to.
     *
     * @return The node this drawable is attached to.
     */
    Node* getNode() const noexcept { return _node; }

    /**
     * Clones the drawable and returns a new drawable as shared_ptr.
     *
     * @param context The clone context for tracking cloned objects.
     * @return The newly created drawable as shared_ptr.
     */
    virtual DrawablePtr cloneDrawable(NodeCloneContext& context) const
    {
        // Default implementation calls the legacy clone() and wraps in shared_ptr
        // Subclasses should override this for proper shared_ptr semantics
        return DrawablePtr(const_cast<Drawable*>(this)->clone(context));
    }

  protected:
    /**
     * Clones the drawable and returns a new drawable.
     *
     * @param context The clone context.
     * @return The newly created drawable.
     * @deprecated Use cloneDrawable() instead for consistent shared_ptr semantics.
     *             This method is kept for backward compatibility with existing subclasses.
     */
    virtual Drawable* clone(NodeCloneContext& context) = 0;

    /**
     * Sets the node this drawable is attached to.
     *
     * @param node The node this drawable is attached to.
     */
    virtual void setNode(Node* node) { _node = node; }

    /**
     * Node this drawable is attached to.
     */
    Node* _node{nullptr};
};

} // namespace tractor
