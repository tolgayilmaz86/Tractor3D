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

namespace tractor
{

class Node;
class NodeCloneContext;

/**
 * Defines a drawable object that can be attached to a Node.
 */
class Drawable
{
    friend class Node;

  public:
    /**
     * Constructor.
     */
    Drawable() = default;

    /**
     * Destructor.
     */
    virtual ~Drawable() = default;

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

  protected:
    /**
     * Clones the drawable and returns a new drawable.
     *
     * @param context The clone context.
     * @return The newly created drawable.
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
