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

#include "math/Matrix.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "renderer/Texture.h"

namespace tractor
{

class Uniform;
class Effect;

/** Shared pointer type for Effect. */
using EffectPtr = std::shared_ptr<Effect>;

/** Weak pointer type for Effect. */
using EffectWeakPtr = std::weak_ptr<Effect>;

/**
 * Defines an effect which can be applied during rendering.
 *
 * An effect essentially wraps an OpenGL program object, which includes the
 * vertex and fragment shader.
 *
 * In the future, this class may be extended to support additional logic that
 * typical effect systems support, such as GPU render state management,
 * techniques and passes.
 */
class Effect : public std::enable_shared_from_this<Effect>
{
  public:
    /**
     * Constructor.
     */
    Effect() = default;

    /**
     * Destructor.
     */
    ~Effect();

    /**
     * Creates an effect using the specified vertex and fragment shader.
     *
     * @param vshPath The path to the vertex shader file.
     * @param fshPath The path to the fragment shader file.
     * @param defines A new-line delimited list of preprocessor defines. May be nullptr.
     *
     * @return The created effect.
     */
    static EffectPtr createFromFile(const std::string& vshPath,
                                    const std::string& fshPath,
                                    const std::string& defines = EMPTY_STRING);

    /**
     * Creates an effect from the given vertex and fragment shader source code.
     *
     * @param vshSource The vertex shader source code.
     * @param fshSource The fragment shader source code.
     * @param defines A new-line delimited list of preprocessor defines. May be nullptr.
     *
     * @return The created effect.
     */
    static EffectPtr createFromSource(const std::string& vshSource,
                                      const std::string& fshSource,
                                      const std::string& defines = EMPTY_STRING);

    /**
     * Returns the unique string identifier for the effect, which is a concatenation of
     * the shader paths it was loaded from.
     */
    const std::string& getId() const noexcept { return _id; }

    /**
     * Returns the vertex attribute handle for the vertex attribute with the specified name.
     *
     * @param name The name of the vertex attribute to return.
     *
     * @return The vertex attribute, or -1 if no such vertex attribute exists.
     */
    VertexAttribute getVertexAttribute(const std::string& name) const;

    /**
     * Returns the uniform handle for the uniform with the specified name.
     *
     * @param name The name of the uniform to return.
     *
     * @return The uniform, or nullptr if no such uniform exists.
     */
    Uniform* getUniform(const std::string& name) const;

    /**
     * Returns the specified active uniform.
     *
     * @param index The index of the uniform to return.
     *
     * @return The uniform, or nullptr if index is invalid.
     */
    Uniform* getUniform(unsigned int index) const;

    /**
     * Returns the number of active uniforms in this effect.
     *
     * @return The number of active uniforms.
     */
    unsigned int getUniformCount() const noexcept { return (unsigned int)_uniforms.size(); }

    /**
     * Sets a float uniform value.
     */
    void setValue(Uniform* uniform, float value);

    /**
     * Sets a float array uniform value.
     */
    void setValue(Uniform* uniform, const float* values, unsigned int count = 1);

    /**
     * Sets an integer uniform value.
     */
    void setValue(Uniform* uniform, int value);

    /**
     * Sets an integer array uniform value.
     */
    void setValue(Uniform* uniform, const int* values, unsigned int count = 1);

    /**
     * Sets a matrix uniform value.
     */
    void setValue(Uniform* uniform, const Matrix& value);

    /**
     * Sets a matrix array uniform value.
     */
    void setValue(Uniform* uniform, const Matrix* values, unsigned int count = 1);

    /**
     * Sets a vector uniform value.
     */
    void setValue(Uniform* uniform, const Vector2& value);

    /**
     * Sets a vector array uniform value.
     */
    void setValue(Uniform* uniform, const Vector2* values, unsigned int count = 1);

    /**
     * Sets a vector uniform value.
     */
    void setValue(Uniform* uniform, const Vector3& value);

    /**
     * Sets a vector array uniform value.
     */
    void setValue(Uniform* uniform, const Vector3* values, unsigned int count = 1);

    /**
     * Sets a vector uniform value.
     */
    void setValue(Uniform* uniform, const Vector4& value);

    /**
     * Sets a vector array uniform value.
     */
    void setValue(Uniform* uniform, const Vector4* values, unsigned int count = 1);

    /**
     * Sets a sampler uniform value.
     */
    void setValue(Uniform* uniform, const Texture::Sampler* sampler);

    /**
     * Sets a sampler array uniform value.
     * @script{ignore}
     */
    void setValue(Uniform* uniform, const Texture::Sampler** values, unsigned int count);

    /**
     * Binds this effect to make it the currently active effect for the rendering system.
     */
    void bind();

    /**
     * Returns the currently bound effect for the rendering system.
     *
     * @return The currently bound effect, or nullptr if no effect is currently bound.
     */
    static Effect* getCurrentEffect();

  private:
    Effect& operator=(const Effect&) = delete;

    static EffectPtr createFromSource(const std::string& vshPath,
                                      const std::string& vshSource,
                                      const std::string& fshPath,
                                      const std::string& fshSource,
                                      const std::string& defines = EMPTY_STRING);

    GLuint _program{ 0 };
    std::string _id{};
    std::map<std::string, VertexAttribute> _vertexAttributes;
    mutable std::map<std::string, std::unique_ptr<Uniform>> _uniforms;
    static Uniform _emptyUniform;
};

/**
 * Represents a uniform variable within an effect.
 */
class Uniform
{
    friend class Effect;

  public:
    /**
     * Returns the name of this uniform.
     */
    const std::string& getName() const noexcept;

    /**
     * Returns the OpenGL uniform type.
     */
    const GLenum getType() const noexcept;

    /**
     * Returns the effect for this uniform.
     */
    Effect* getEffect() const noexcept;

    /**
     * Default constructor.
     */
    Uniform();

    /**
     * Destructor.
     */
    ~Uniform();

  private:
    Uniform(const Uniform& copy);
    Uniform& operator=(const Uniform&) = delete;

    std::string _name;
    GLint _location;
    GLenum _type;
    unsigned int _index;
    Effect* _effect;
};

} // namespace tractor
