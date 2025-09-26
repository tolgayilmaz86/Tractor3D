#include "pch.h"
#include "renderer/Material.h"
#include "framework/FileSystem.h"
#include "graphics/Effect.h"
#include "renderer/Technique.h"
#include "renderer/Pass.h"
#include "scene/Properties.h"
#include "scene/Node.h"

namespace tractor
{
	Material::~Material()
	{
		// Destroy all the techniques.
		std::ranges::for_each(_techniques, [](auto& technique) {
			SAFE_RELEASE(technique);
			});
		_techniques.clear();

	}

	Material* Material::create(const std::string& url)
	{
		return create(url, (PassCallback)nullptr, nullptr);
	}

	Material* Material::create(const std::string& url, PassCallback callback, void* cookie)
	{
		// Load the material properties from file.
		Properties* properties = Properties::create(url);
		if (properties == nullptr)
		{
			GP_WARN("Failed to create material from file: %s", url);
			return nullptr;
		}

		Material* material = create(properties->getNamespace().length() > 0 ? properties : properties->getNextNamespace(), callback, cookie);
		SAFE_DELETE(properties);

		return material;
	}

	Material* Material::create(Properties* materialProperties)
	{
		return create(materialProperties, (PassCallback)nullptr, nullptr);
	}

	Material* Material::create(Properties* materialProperties, PassCallback callback, void* cookie)
	{
		// Check if the Properties is valid and has a valid namespace.
		if (!materialProperties || materialProperties->getNamespace() != "material")
		{
			GP_ERROR("Properties object must be non-null and have namespace equal to 'material'.");
			return nullptr;
		}

		// Create new material from the file passed in.
		Material* material = new Material();

		// Load uniform value parameters for this material.
		loadRenderState(material, materialProperties);

		materialProperties->rewind();
		// Go through all the material properties and create techniques under this material.
		Properties* techniqueProperties = nullptr;
		while ((techniqueProperties = materialProperties->getNextNamespace()))
		{
			if (techniqueProperties->getNamespace() == "technique")
			{
				if (!loadTechnique(material, techniqueProperties, callback, cookie))
				{
					GP_ERROR("Failed to load technique for material.");
					SAFE_RELEASE(material);
					return nullptr;
				}
			}
		}

		// Set the current technique to the first found technique.
		if (material->getTechniqueCount() > 0)
		{
			Technique* t = material->getTechniqueByIndex(0);
			if (t)
				material->_currentTechnique = t;
		}
		return material;
	}

	Material* Material::create(Effect* effect)
	{
		assert(effect);

		// Create a new material with a single technique and pass for the given effect.
		Material* material = new Material();

		const auto& technique = material->_techniques.emplace_back(new Technique(EMPTY_STRING, material));

		const auto& pass = technique->_passes.emplace_back(new Pass(EMPTY_STRING, technique));
		pass->_effect = effect;
		effect->addRef();

		material->_currentTechnique = technique;

		return material;
	}

	Material* Material::create(const std::string& vshPath, const std::string& fshPath, const std::string& defines)
	{
		// Create a new material with a single technique and pass for the given effect
		Material* material = new Material();

		const auto& technique = material->_techniques.emplace_back(new Technique(EMPTY_STRING, material));
		technique->addRef();
		Pass* pass = new Pass(EMPTY_STRING, technique);
		if (!pass->initialize(vshPath, fshPath, defines))
		{
			GP_WARN("Failed to create pass for material: vertexShader = %s, fragmentShader = %s, defines = %s", vshPath.c_str(), fshPath.c_str(), defines.c_str());
			SAFE_RELEASE(pass);
			SAFE_RELEASE(material);
			return nullptr;
		}
		technique->_passes.push_back(pass);

		material->_currentTechnique = technique;

		return material;
	}

	unsigned int Material::getTechniqueCount() const
	{
		return (unsigned int)_techniques.size();
	}

	Technique* Material::getTechniqueByIndex(unsigned int index) const
	{
		assert(index < _techniques.size());
		return _techniques[index];
	}

	Technique* Material::getTechnique(const std::string& id) const
	{
		for (size_t i = 0, count = _techniques.size(); i < count; ++i)
		{
			Technique* t = _techniques[i];
			assert(t);
			if (t->getId() == id) return t;
		}

		return nullptr;
	}

	Technique* Material::getTechnique() const
	{
		return _currentTechnique;
	}

	void Material::setTechnique(const std::string& id)
	{
		Technique* t = getTechnique(id);
		if (t) _currentTechnique = t;
	}

	void Material::setNodeBinding(Node* node)
	{
		RenderState::setNodeBinding(node);

		for (size_t i = 0, count = _techniques.size(); i < count; ++i)
			_techniques[i]->setNodeBinding(node);
	}

	Material* Material::clone(NodeCloneContext& context) const
	{
		Material* material = new Material();
		RenderState::cloneInto(material, context);

		for (const auto& technique : _techniques)
		{
			assert(technique);
			Technique* techniqueClone = technique->clone(material, context);
			material->_techniques.push_back(techniqueClone);
			if (_currentTechnique == technique)
			{
				material->_currentTechnique = techniqueClone;
			}
		}
		return material;
	}

	bool Material::loadTechnique(Material* material, Properties* techniqueProperties, PassCallback callback, void* cookie)
	{
		assert(material);
		assert(techniqueProperties);

		// Create a new technique.
		// Add the new technique to the material.
		auto& technique = material->_techniques.emplace_back(new Technique(techniqueProperties->getId(), material));

		// Load uniform value parameters for this technique.
		loadRenderState(technique, techniqueProperties);

		// Go through all the properties and create passes under this technique.
		techniqueProperties->rewind();
		Properties* passProperties = nullptr;
		while ((passProperties = techniqueProperties->getNextNamespace()))
		{
			if (passProperties->getNamespace() == "pass")
			{
				// Create and load passes.
				if (!loadPass(technique, passProperties, callback, cookie))
				{
					GP_ERROR("Failed to create pass for technique.");
					SAFE_RELEASE(technique);
					return false;
				}
			}
		}

		return true;
	}

	bool Material::loadPass(Technique* technique, Properties* passProperties, PassCallback callback, void* cookie)
	{
		assert(passProperties);
		assert(technique);

		// Fetch shader info required to create the effect of this technique.
		auto vertexShaderPath = passProperties->getString("vertexShader");
		auto fragmentShaderPath = passProperties->getString("fragmentShader");
		auto passDefines = passProperties->getString("defines");

		// Create the pass
		Pass* pass = new Pass(passProperties->getId(), technique);

		// Load render state.
		loadRenderState(pass, passProperties);

		// If a pass callback was specified, call it and add the result to our list of defines
		std::string allDefines = !passDefines.empty() ? passDefines : "";
		if (callback)
		{
			std::string customDefines = callback(pass, cookie);
			if (customDefines.length() > 0)
			{
				if (allDefines.length() > 0)
					allDefines += ';';
				allDefines += customDefines;
			}
		}

		// Initialize/compile the effect with the full set of defines
		if (!pass->initialize(vertexShaderPath, fragmentShaderPath, allDefines))
		{
			GP_WARN("Failed to create pass for technique.");
			SAFE_RELEASE(pass);
			return false;
		}

		// Add the new pass to the technique.
		technique->_passes.push_back(pass);

		return true;
	}

	static bool isMaterialKeyword(const std::string& str)
	{
#define MATERIAL_KEYWORD_COUNT 3
		static std::string reservedKeywords[MATERIAL_KEYWORD_COUNT] =
		{
			"vertexShader",
			"fragmentShader",
			"defines"
		};
		for (unsigned int i = 0; i < MATERIAL_KEYWORD_COUNT; ++i)
		{
			if (reservedKeywords[i] == str)
			{
				return true;
			}
		}
		return false;
	}

	static Texture::Filter parseTextureFilterMode(const std::string& str, Texture::Filter defaultValue)
	{
		if (str.empty())
		{
			GP_ERROR("Texture filter mode string must be non-null and non-empty.");
			return defaultValue;
		}
		else if (str == "NEAREST")
		{
			return Texture::NEAREST;
		}
		else if (str == "LINEAR")
		{
			return Texture::LINEAR;
		}
		else if (str == "NEAREST_MIPMAP_NEAREST")
		{
			return Texture::NEAREST_MIPMAP_NEAREST;
		}
		else if (str == "LINEAR_MIPMAP_NEAREST")
		{
			return Texture::LINEAR_MIPMAP_NEAREST;
		}
		else if (str == "NEAREST_MIPMAP_LINEAR")
		{
			return Texture::NEAREST_MIPMAP_LINEAR;
		}
		else if (str == "LINEAR_MIPMAP_LINEAR")
		{
			return Texture::LINEAR_MIPMAP_LINEAR;
		}
		else
		{
			GP_ERROR("Unsupported texture filter mode string ('%s').", str);
			return defaultValue;
		}
	}

	static Texture::Wrap parseTextureWrapMode(const std::string& str, Texture::Wrap defaultValue)
	{
		if (str.empty())
		{
			GP_ERROR("Texture wrap mode string must be non-null and non-empty.");
			return defaultValue;
		}
		else if (str == "REPEAT")
		{
			return Texture::REPEAT;
		}
		else if (str == "CLAMP")
		{
			return Texture::CLAMP;
		}
		else
		{
			GP_ERROR("Unsupported texture wrap mode string ('%s').", str);
			return defaultValue;
		}
	}

	void Material::loadRenderState(RenderState* renderState, Properties* properties)
	{
		assert(renderState);
		assert(properties);

		// Rewind the properties to start reading from the start.
		properties->rewind();

		while (auto property = properties->getNextProperty())
		{
			const auto& name = property->name;

			if (isMaterialKeyword(name))
				continue; // keyword - skip

			switch (properties->getType())
			{
			case Properties::NUMBER:
				renderState->getParameter(name)->setValue(properties->getFloat());
				break;
			case Properties::VECTOR2:
			{
				Vector2 vector2;
				if (properties->getVector2(EMPTY_STRING, &vector2))
				{
					renderState->getParameter(name)->setValue(vector2);
				}
			}
			break;
			case Properties::VECTOR3:
			{
				Vector3 vector3;
				if (properties->getVector3(EMPTY_STRING, &vector3))
				{
					renderState->getParameter(name)->setValue(vector3);
				}
			}
			break;
			case Properties::VECTOR4:
			{
				Vector4 vector4;
				if (properties->getVector4(EMPTY_STRING, &vector4))
				{
					renderState->getParameter(name)->setValue(vector4);
				}
			}
			break;
			case Properties::MATRIX:
			{
				Matrix matrix;
				if (properties->getMatrix(EMPTY_STRING, &matrix))
				{
					renderState->getParameter(name)->setValue(matrix);
				}
			}
			break;
			default:
			{
				// Assume this is a parameter auto-binding.
				renderState->setParameterAutoBinding(name, properties->getString());
			}
			break;
			}
		}

		// Rewind the properties to start reading from the start.
		properties->rewind();

		// Iterate through all child namespaces searching for samplers and render state blocks.
		Properties* ns;
		while ((ns = properties->getNextNamespace()))
		{
			std::string name;
			if (ns->getNamespace() == "sampler")
			{
				// Read the texture uniform name.
				name = ns->getId();
				if (name.empty())
				{
					GP_ERROR("Texture sampler is missing required uniform name.");
					continue;
				}

				// Get the texture path.
				std::string path;
				if (!ns->getPath("path", &path))
				{
					GP_ERROR("Texture sampler '%s' is missing required image file path.", name);
					continue;
				}

				// Read texture state (booleans default to 'false' if not present).
				bool mipmap = ns->getBool("mipmap");
				Texture::Wrap wrapS = parseTextureWrapMode(ns->getString("wrapS"), Texture::REPEAT);
				Texture::Wrap wrapT = parseTextureWrapMode(ns->getString("wrapT"), Texture::REPEAT);
				Texture::Wrap wrapR = Texture::REPEAT;
				if (ns->exists("wrapR"))
				{
					wrapR = parseTextureWrapMode(ns->getString("wrapR"), Texture::REPEAT);
				}
				Texture::Filter minFilter = parseTextureFilterMode(ns->getString("minFilter"), mipmap ? Texture::NEAREST_MIPMAP_LINEAR : Texture::LINEAR);
				Texture::Filter magFilter = parseTextureFilterMode(ns->getString("magFilter"), Texture::LINEAR);

				// Set the sampler parameter.
				assert(renderState->getParameter(name));
				Texture::Sampler* sampler = renderState->getParameter(name)->setValue(path, mipmap);
				if (sampler)
				{
					sampler->setWrapMode(wrapS, wrapT, wrapR);
					sampler->setFilterMode(minFilter, magFilter);
				}
			}
			else if (ns->getNamespace() == "renderState")
			{
				while (auto property = ns->getNextProperty())
				{
					assert(renderState->getStateBlock());
					renderState->getStateBlock()->setState(property->name, ns->getString());
				}
			}
		}
	}

}
