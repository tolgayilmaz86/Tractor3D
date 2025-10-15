# Tractor3D: Modern C++ Game Engine

A comprehensive modernization of the Gameplay3D engine, transforming legacy C++98/11 code into modern C++20. This project represents a complete overhaul focused on safety, performance, and maintainability while leveraging the latest C++ features.

[![License](https://img.shields.io/badge/License-Apache%202.0-blue.svg)](LICENSE)
[![Platform](https://img.shields.io/badge/Platform-Windows%20x64-lightgrey.svg)]()
[![C++](https://img.shields.io/badge/C%2B%2B-20-blue.svg)]()

## 🎯 Project Goals

- **Safety First**: Eliminate undefined behavior and memory leaks through modern C++ idioms
- **Performance**: Leverage compiler optimizations and efficient standard library algorithms
- **Maintainability**: Clean, readable code that's easy to understand and extend
- **Modern Standards**: Utilize C++20 features for better expressiveness and type safety

---

## 🚀 Modernization Categories

### 1. Foundational Safety and Readability

#### Null Pointer Safety
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
Game* __gameInstance = NULL;
if (ptr == 0) {
    // handle null case
}
```

</td>
<td>

```cpp
Game* __gameInstance = nullptr;
if (ptr == nullptr) {
    // handle null case
}
```

</td>
</tr>
</table>

**Benefits**: Type-safe null pointers prevent ambiguity between null pointers and integer zeros.

#### In-Class Member Initialization
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
class GameEntity {
private:
    int _health;
    float* _position;
    bool _active;
public:
    GameEntity();
};

GameEntity::GameEntity() 
    : _health(100)
    , _position(nullptr)
    , _active(false) {
}
```

</td>
<td>

```cpp
class GameEntity {
private:
    int _health{100};
    float* _position{nullptr};
    bool _active{false};
public:
    GameEntity() = default;
};
```

</td>
</tr>
</table>

**Benefits**: Clearer intent, reduced constructor boilerplate, impossible to forget initialization.

#### Explicit Special Member Functions
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
class NonCopyable {
private:
    NonCopyable(const NonCopyable&);
    NonCopyable& operator=(const NonCopyable&);
};
```

</td>
<td>

```cpp
class NonCopyable {
public:
    NonCopyable(const NonCopyable&) = delete;
    NonCopyable& operator=(const NonCopyable&) = delete;
    NonCopyable() = default;
    ~NonCopyable() = default;
};
```

</td>
</tr>
</table>

**Benefits**: Explicit intent, better error messages, compiler optimizations.

#### Virtual Function Safety
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
class Renderer : public BaseRenderer {
    virtual void render();
    virtual void cleanup();
};
```

</td>
<td>

```cpp
class Renderer final : public BaseRenderer {
    void render() override;
    void cleanup() override;
};
```

</td>
</tr>
</table>

**Benefits**: Compile-time verification of overrides, performance optimizations with `final`.

#### Strong Type Safety
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
enum EntityType {
    PLAYER,
    ENEMY,
    ITEM
};

void processEntity(int type) {
    if (type == PLAYER) { /* ... */ }
}
```

</td>
<td>

```cpp
enum class EntityType {
    Player,
    Enemy,
    Item
};

void processEntity(EntityType type) {
    if (type == EntityType::Player) { /* ... */ }
}
```

</td>
</tr>
</table>

**Benefits**: Prevents implicit conversions, eliminates naming conflicts, clearer scope.

#### Modern String Handling
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
Node* Scene::addNode(const char* id) {
    if (id == nullptr) return nullptr;
    // ... string operations
}
```

</td>
<td>

```cpp
Node* Scene::addNode(const std::string& id) {
    if (id.empty()) return nullptr;
    // ... string operations
}
```

</td>
</tr>
</table>

**Benefits**: Automatic memory management, no null checks needed, rich string interface.

### 2. Modern C++ Idioms

#### Range-Based Loops
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
for (auto it = entities.begin(); 
     it != entities.end(); ++it) {
    it->update();
}

for (size_t i = 0; i < entities.size(); ++i) {
    entities[i].render();
}
```

</td>
<td>

```cpp
for (auto& entity : entities) {
    entity.update();
}

for (const auto& entity : entities) {
    entity.render();
}
```

</td>
</tr>
</table>

**Benefits**: Cleaner syntax, no iterator errors, clear iteration intent.

#### Type Deduction
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
std::map<std::string, Entity*>::const_iterator it = 
    entityMap.find("player");

std::vector<std::shared_ptr<Component>> components = 
    entity.getComponents();
```
</td>
<td>

```cpp
auto it = entityMap.find("player");

auto components = entity.getComponents();
```
</td>
</tr>
</table>

**Benefits**: Reduced verbosity, easier refactoring, maintains type safety.

#### Standard Library Algorithms
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
float cpX = center.x;
const Vector3& boxMin = box.min;
const Vector3& boxMax = box.max;

if (center.x < boxMin.x) {
    cpX = boxMin.x;
} else if (center.x > boxMax.x) {
    cpX = boxMax.x;
}
```

</td>
<td>

```cpp
float cpX = std::clamp(center.x, box.min.x, box.max.x);
```
</td>
</tr>
</table>

**Benefits**: Compiler optimizations, less error-prone, clearer intent.

#### Modern Function Signatures
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
void Matrix::createOrthographic(
    float width, float height, 
    float zNear, float zFar, 
    Matrix* dst) {
    // modify dst
}
```

</td>
<td>

```cpp
Matrix Matrix::createOrthographic(
    float width, float height, 
    float zNear, float zFar) {
    // return result
}
```

</td>
</tr>
</table>

**Benefits**: Clear ownership, return value optimization, functional style.

### 3. Memory and Resource Management

#### Smart Pointers
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
class Game {
    AnimationController* _animController;
public:
    Game() : _animController(nullptr) {}
    ~Game() { 
        SAFE_DELETE(_animController); 
    }
    
    void initialize() {
        _animController = new AnimationController();
    }
};
```

</td>
<td>

```cpp
class Game {
    std::unique_ptr<AnimationController> _animController;
public:
    Game() = default;
    ~Game() = default; // Automatic cleanup
    
    void initialize() {
        _animController = 
            std::make_unique<AnimationController>();
    }
};
```

</td>
</tr>
</table>

**Benefits**: Automatic memory management, exception safety, clear ownership semantics.

### 4. Function Correctness

#### Compile-Time Constants
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
#define MAX_ENTITIES 1024
#define PI 3.14159f

// In global scope
static const int BUFFER_SIZE = 512;
```

</td>
<td>

```cpp
constexpr int MAX_ENTITIES = 1024;
constexpr float PI = 3.14159f;

constexpr int BUFFER_SIZE = 512;
```

</td>
</tr>
</table>

**Benefits**: Type safety, proper scoping, debugger support, compile-time evaluation.

#### Function Qualifiers
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
class Game {
public:
    static Game* getInstance();
    bool isVsync();
    Vector3 getPosition();
};
```

</td>
<td>

```cpp
class Game {
public:
    static Game* getInstance() noexcept;
    bool isVsync() const noexcept;
    const Vector3& getPosition() const noexcept;
};
```

</td>
</tr>
</table>

**Benefits**: Compiler optimizations, const correctness, clear API contracts.

### 5. Advanced Modern C++ Features

#### Ranges and Views
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
int maxFocusIndex = 0;
for (size_t i = 0; i < _controls.size(); ++i) {
    if (_controls[i]->_focusIndex > maxFocusIndex) {
        maxFocusIndex = _controls[i]->_focusIndex;
    }
}
```

</td>
<td>

```cpp
int maxFocusIndex = 0;
if (!_controls.empty()) {
    maxFocusIndex = std::ranges::max(
        _controls | std::views::transform(
            [](const Control* c) {
                return c->getFocusIndex();
            }));
}
```

</td>
</tr>
</table>

#### Standard Library Power
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
for (unsigned int i = 0; i < data->vertexCount; i++) {
    indexData[i] = i;
}

// Check if any contact exists
for (size_t i = 0; i < MAX_CONTACTS; ++i) {
    if (_contactIndices[i]) return true;
}
return false;
```
</td>
<td>

```cpp
std::ranges::copy(
    std::views::iota(0u, data->vertexCount), 
    indexData);

// Check if any contact exists
return std::ranges::any_of(_contactIndices, 
    [](bool contact) { return contact; });
```
</td>
</tr>
</table>

#### Optional Values
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
Entity* findEntity(const std::string& name) {
    // ... search logic
    return nullptr; // if not found
}

// Usage
Entity* entity = findEntity("player");
if (entity != nullptr) {
    entity->update();
}
```

</td>
<td>

```cpp
std::optional<Entity> findEntity(const std::string& name) {
    // ... search logic
    return std::nullopt; // if not found
}

// Usage
if (auto entity = findEntity("player")) {
    entity->update();
}
```

</td>
</tr>
</table>

#### Type-Safe Variants
<table>
<tr>
<th>Before (Legacy)</th>
<th>After (Modern C++)</th>
</tr>
<tr>
<td>

```cpp
union PropertyValue {
    int intValue;
    float floatValue;
    char* stringValue;
};

struct Property {
    PropertyType type;
    PropertyValue value;
};
```

</td>
<td>

```cpp
using PropertyValue = std::variant<
    int, 
    float, 
    std::string
>;

struct Property {
    PropertyValue value;
};

// Usage with visitor pattern
std::visit([](const auto& val) {
    std::cout << val << '\n';
}, property.value);
```

</td>
</tr>
</table>

---

## 🏗️ Project Structure

```
Tractor3Dlib/
├── res/              # Shaders, logo, materials, ui design
├── include/          # Same structure with source files
├── src/
│   ├── ai/           # Artificial Intelligence agents
│   ├── animation/    # Animation systems
│   ├── audio/        # Audio systems
│   ├── framework/    # File system, platform and main game components
│   ├── graphics/     # Ray tracing, shapes, light, texcture, material etc.
│   ├── input/        # Gamepad, keyboard 
│   ├── math/         # Math operations
│   ├── physics/      # Physics simulation
│   ├── renderer/     # Rendering and visual systems
│   ├── scene/        # Scene construction 
│   ├── scripting/    # Lua scripting
│   ├── ui/           # Graphical user interface 
│   └── utils/        # Utility classes and functions
├── samples/          # Executable samples
```

## 🔧 Building

### Prerequisites
- Visual Studio 2022 (17.0 or later)
- Windows 10/11 x64
- C++20 compatible compiler

### Build Instructions
- Install vcpkg in your prefered directory.
- Clone the repository.
- Call the following commands within the project directory
```bash
git clone https://github.com/your-repo/Tractor3D.git
cd Tractor3D

${VCPKG_DIR}\vcpkg integrate install
${VCPKG_DIR}\vcpkg install bullet3
${VCPKG_DIR}\vcpkg install lua:x64-windows
${VCPKG_DIR}\vcpkg install libpng:x64-windows
${VCPKG_DIR}\vcpkg install glew:x64-windows
${VCPKG_DIR}\vcpkg install libvorbis:x64-windows
${VCPKG_DIR}\vcpkg install openal-soft:x64-windows
```

## 📋 Modernization Checklist

- [x] Replace NULL with nullptr
- [x] Implement in-class member initialization
- [x] Use = default and = delete for special members
- [x] Add override and final specifiers
- [x] Convert to enum class
- [x] Replace C-strings with std::string
- [x] Implement smart pointers (unique_ptr/shared_ptr)
- [x] Use range-based for loops
- [x] Apply auto type deduction
- [x] Leverage std algorithms
- [x] Use constexpr for compile-time constants
- [x] Add const and noexcept qualifiers
- [ ] Implement concepts for template constraints
- [ ] Utilize std::span for array views
- [ ] Apply std::optional for nullable returns
- [ ] Use std::variant for type-safe unions
- [ ] Add comprehensive testing
- [ ] Add logging system
- [ ] Consider ECS architecture
- [ ] C# as scripting language


## 🤝 Contributing

We welcome contributions! Please see our [Contributing Guidelines](CONTRIBUTING.md) for details on:
- Code style and formatting
- Testing requirements
- Pull request process
- Issue reporting

## 📄 License

This project is licensed under the Apache License 2.0 - see the [LICENSE](LICENSE) file for details.

## 🙏 Acknowledgments

- Original Gameplay3D team for the foundation
- Modern C++ community for best practices
- Contributors and testers

---

*Tractor3D represents the evolution of game engine development, embracing modern C++ for safer, faster, and more maintainable code.*