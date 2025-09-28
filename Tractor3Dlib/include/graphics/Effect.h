#pragma once

#include "math/Matrix.h"
#include "math/Vector2.h"
#include "math/Vector3.h"
#include "math/Vector4.h"
#include "renderer/Texture.h"
#include "utils/Ref.h"

namespace tractor
{

class Uniform;

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
class Effect : public Ref
{
  public:
    /**
     * Hidden constructor (use createEffect instead).
     */
    Effect();

    /**
     * Hidden destructor (use destroyEffect instead).
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
    static Effect* createFromFile(const std::string& vshPath,
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
    static Effect* createFromSource(const std::string& vshSource,
                                    const std::string& fshSource,
                                    const std::string& defines = EMPTY_STRING);

    /**
     * Returns the unique string identifier for the effect, which is a concatenation of
     * the shader paths it was loaded from.
     */
    const std::string& getId() const;

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
    unsigned int getUniformCount() const;

    /**
     * Sets a float uniform value.
     *
     * @param uniform The uniform to set.
     * @param value The float value to set.
     */
    void setValue(Uniform* uniform, float value);

    /**
     * Sets a float array uniform value.
     *
     * @param uniform The uniform to set.
     * @param values The array to set.
     * @param count The number of elements in the array.
     */
    void setValue(Uniform* uniform, const float* values, unsigned int count = 1);

    /**
     * Sets an integer uniform value.
     *
     * @param uniform The uniform to set.
     * @param value The value to set.
     */
    void setValue(Uniform* uniform, int value);

    /**
     * Sets an integer array uniform value.
     *
     * @param uniform The uniform to set.
     * @param values The array to set.
     * @param count The number of elements in the array.
     */
    void setValue(Uniform* uniform, const int* values, unsigned int count = 1);

    /**
     * Sets a matrix uniform value.
     *
     * @param uniform The uniform to set.
     * @param value The value to set.
     */
    void setValue(Uniform* uniform, const Matrix& value);

    /**
     * Sets a matrix array uniform value.
     *
     * @param uniform The uniform to set.
     * @param values The array to set.
     * @param count The number of elements in the array.
     */
    void setValue(Uniform* uniform, const Matrix* values, unsigned int count = 1);

    /**
     * Sets a vector uniform value.
     *
     * @param uniform The uniform to set.
     * @param value The value to set.
     */
    void setValue(Uniform* uniform, const Vector2& value);

    /**
     * Sets a vector array uniform value.
     *
     * @param uniform The uniform to set.
     * @param values The array to set.
     * @param count The number of elements in the array.
     */
    void setValue(Uniform* uniform, const Vector2* values, unsigned int count = 1);

    /**
     * Sets a vector uniform value.
     *
     * @param uniform The uniform to set.
     * @param value The value to set.
     */
    void setValue(Uniform* uniform, const Vector3& value);

    /**
     * Sets a vector array uniform value.
     *
     * @param uniform The uniform to set.
     * @param values The array to set.
     * @param count The number of elements in the array.
     */
    void setValue(Uniform* uniform, const Vector3* values, unsigned int count = 1);

    /**
     * Sets a vector uniform value.
     *
     * @param uniform The uniform to set.
     * @param value The value to set.
     */
    void setValue(Uniform* uniform, const Vector4& value);

    /**
     * Sets a vector array uniform value.
     *
     * @param uniform The uniform to set.
     * @param values The array to set.
     * @param count The number of elements in the array.
     */
    void setValue(Uniform* uniform, const Vector4* values, unsigned int count = 1);

    /**
     * Sets a sampler uniform value.
     *
     * @param uniform The uniform to set.
     * @param sampler The sampler to set.
     */
    void setValue(Uniform* uniform, const Texture::Sampler* sampler);

    /**
     * Sets a sampler array uniform value.
     *
     * @param uniform The uniform to set.
     * @param values The sampler array to set.
     * @param count The number of elements in the array.
     *
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
    /**
     * Hidden copy assignment operator.
     */
    Effect& operator=(const Effect&) = delete;

    static Effect* createFromSource(const std::string& vshPath,
                                    const std::string& vshSource,
                                    const std::string& fshPath,
                                    const std::string& fshSource,
                                    const std::string& defines = EMPTY_STRING);

    GLuint _program;
    std::string _id;
    std::map<std::string, VertexAttribute> _vertexAttributes;
    mutable std::map<std::string, Uniform*> _uniforms;
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
     *
     * @return The name of the uniform.
     */
    const std::string& getName() const;

    /**
     * Returns the OpenGL uniform type.
     *
     * @return The OpenGL uniform type.
     */
    const GLenum getType() const;

    /**
     * Returns the effect for this uniform.
     *
     * @return The uniform's effect.
     */
    Effect* getEffect() const;

  private:
    /**
     * Constructor.
     */
    Uniform();

    /**
     * Copy constructor.
     */
    Uniform(const Uniform& copy);

    /**
     * Destructor.
     */
    ~Uniform();

    /**
     * Hidden copy assignment operator.
     */
    Uniform& operator=(const Uniform&) = delete;

    std::string _name;
    GLint _location;
    GLenum _type;
    unsigned int _index;
    Effect* _effect;
};

} // namespace tractor
