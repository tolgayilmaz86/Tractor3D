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

/**
 * Defines the base class for game objects that require lifecycle management.
 *
 * @deprecated This class is deprecated. Migrate to std::shared_ptr / std::weak_ptr ownership semantics.
 * 
 * ## Migration Guide: From Ref to Smart Pointers
 * 
 * When migrating a class from Ref to smart pointers, follow these steps:
 * 
 * ### 1. Remove Ref Inheritance
 * ```cpp
 * // Before:
 * class MyClass : public Ref { ... };
 * 
 * // After:
 * class MyClass : public std::enable_shared_from_this<MyClass> { ... };
 * ```
 * 
 * ### 2. Make Destructor Public
 * ```cpp
 * // Before:
 * private:
 *     virtual ~MyClass() = default;  // Hidden, use SAFE_RELEASE
 * 
 * // After:
 * public:
 *     virtual ~MyClass() = default;
 * ```
 * 
 * ### 3. Update Factory Methods
 * ```cpp
 * // Before:
 * static MyClass* create();
 * MyClass* MyClass::create() { return new MyClass(); }
 * 
 * // After:
 * static std::shared_ptr<MyClass> create();
 * std::shared_ptr<MyClass> MyClass::create() { 
 *     return std::shared_ptr<MyClass>(new MyClass()); 
 * }
 * ```
 * 
 * ### 4. Update Member Storage
 * ```cpp
 * // Before:
 * MyClass* _member{ nullptr };
 * 
 * // After (for owned resources):
 * std::shared_ptr<MyClass> _member;
 * 
 * // After (for weak references to avoid cycles):
 * std::weak_ptr<MyClass> _memberWeak;
 * ```
 * 
 * ### 5. Remove addRef()/release() Calls
 * ```cpp
 * // Before:
 * void setMember(MyClass* member) {
 *     if (_member) _member->release();
 *     _member = member;
 *     if (_member) _member->addRef();
 * }
 * 
 * // After:
 * void setMember(const std::shared_ptr<MyClass>& member) {
 *     _member = member;  // shared_ptr handles ref counting
 * }
 * ```
 * 
 * ### 6. Replace SAFE_RELEASE
 * ```cpp
 * // Before:
 * SAFE_RELEASE(_member);
 * 
 * // After:
 * _member.reset();  // or just let destructor handle it
 * ```
 * 
 * ### 7. Update Linked Lists to Vector
 * ```cpp
 * // Before (intrusive linked list):
 * MyClass* _first{ nullptr };
 * MyClass* _next{ nullptr };  // in MyClass
 * 
 * // After:
 * std::vector<std::shared_ptr<MyClass>> _items;
 * ```
 * 
 * ### Classes Still Using Ref (Migration Pending)
 * The following classes still inherit from Ref and need to be migrated:
 * - Animation, AnimationClip
 * - AudioBuffer, AudioSource
 * - Effect, HeightField, Light, Model, ParticleEmitter, Sprite, Terrain, TileSet
 * - PhysicsCollisionShape
 * - Camera, Font, FrameBuffer, RenderState, RenderState::StateBlock
 * - MaterialParameter::MethodBinding
 * - Text, Texture, Texture::Sampler, VertexAttributeBinding
 * - Bundle, Scene, Node
 * - Control, Layout, Theme (and its nested classes)
 * - ThemeStyle::Overlay
 * 
 * ### Successfully Migrated Classes
 * - AIAgent (uses std::shared_ptr)
 */
class [[deprecated("Ref is deprecated. Migrate to std::shared_ptr / std::weak_ptr ownership semantics.")]] Ref
{
  public:
    /**
     * Increments the reference count of this object.
     *
     * The release() method must be called when the caller relinquishes its
     * handle to this object in order to decrement the reference count.
     */
    void addRef();

    /**
     * Decrements the reference count of this object.
     *
     * When an object is initially created, its reference count is set to 1.
     * Calling addRef() will increment the reference and calling release()
     * will decrement the reference count. When an object reaches a
     * reference count of zero, the object is destroyed.
     */
    void release();

    /**
     * Returns the current reference count of this object.
     *
     * @return This object's reference count.
     */
    unsigned int getRefCount() const noexcept { return _refCount; }

  protected:
    /**
     * Constructor.
     */
    Ref();

    /**
     * Copy constructor.
     *
     * @param copy The Ref object to copy.
     */
    Ref(const Ref& copy);

    /**
     * Destructor.
     */
    virtual ~Ref();

  private:
    unsigned int _refCount;

    // Memory leak diagnostic data (only included when GP_USE_MEM_LEAK_DETECTION is defined)
#ifdef GP_USE_MEM_LEAK_DETECTION
    friend class Game;
    static void printLeaks();
    void* __record;
#endif
};

} // namespace tractor
