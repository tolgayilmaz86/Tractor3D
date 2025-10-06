# Revive and Modernize an Old Game Engine

This document outlines the roadmap and best practices for modernizing the old game engine known as Gameplay3d codebase from its original C++98/11 style to modern C++20. 

With regard to Apache 2.0 license this codebase has substantially changed the old engine with some additions and ease of use. 
Also to keep things simple onl Windows x64 platforms will be supported. 

The goal is to improve code readability, maintainability, and performance by using most modern c++ language features and relying on the STD as much as possible.

---

## Categorized Modernization Tasks

### 1. Foundational Safety and Readability

These are simple, high-impact changes that make the code safer and more explicit.

#### Use `nullptr` for Null Pointers

-   **What:** Replace all uses of `NULL` or `0` for pointers with the `nullptr` keyword.
-   **Why:** `nullptr` is a type-safe keyword for pointers, whereas `NULL` is just a macro for `0`. This prevents ambiguity and potential bugs where a null pointer could be confused with an integer.

-   **Before:**
    ```cpp
    Game* __gameInstance = NULL;
    ```
-   **After:**
    ```cpp
    Game* __gameInstance = nullptr;
    ```

#### In-Class Member Initialization

-   **What:** Initialize member variables at their declaration point if the value is a fixed default.
-   **Why:** This makes the class definition clearer by co-locating the declaration and default value. It reduces boilerplate in constructors.

-   **Before:**
    ```cpp
    class Class {
    private:
        int _member;
        Pointer* _ptr;
    public:
        Class();
    };

    Class::Class() : _member(0), _ptr(nullptr) {}
    ```
-   **After:**
    ```cpp
    class Class {
    private:
        int _member { 0 };
        Pointer* _ptr { nullptr };
    public:
        Class() = default; // See below
    };
    ```

#### Use `=default` and `=delete`

-   **What:** Use `= default` to explicitly tell the compiler to generate a default special member function (like a constructor or destructor). Use `= delete` to prevent the compiler from generating one.
-   **Why:** It clearly communicates intent. `= default` is often more efficient than an empty user-written implementation. `= delete` prevents objects from being copied or moved when they shouldn't be.

-   **Before:**
    ```cpp
    class NonCopyable {
    private:
        NonCopyable(const NonCopyable&);
        NonCopyable& operator=(const NonCopyable&);
    };
    ```
-   **After:**
    ```cpp
    class NonCopyable {
        NonCopyable(const NonCopyable&) = delete;
        NonCopyable& operator=(const NonCopyable&) = delete;
    };
    ```
  - **Note:** This also reuires to remove constructor initialization parameters by initializing them using in class member initialization. such as assigning the value where the member variable is defined using {} operators.
#### Use `override` and `final`

-   **What:** Add `override` to virtual functions in derived classes. Use `final` on virtual functions that should not be overridden further, or on classes that should not be inherited from.
-   **Why:** `override` causes the compiler to verify that the function is actually overriding a base class method, preventing subtle bugs from signature mismatches. `final` can improve performance by allowing the compiler to perform de-virtualization.

-   **Before:**
    ```cpp
    class MyWidget : public Widget {
        virtual void draw();
    };
    ```
-   **After:**
    ```cpp
    class MyWidget final : public Widget { // Class cannot be inherited from
        void draw() override; // Compiler checks this overrides a base method
    };
    ```
  - 
#### Use `enum class` for Strongly Typed Enums
- **What:** Replace traditional enums with `enum class` to create strongly typed enumerations.
- **Why:** `enum class` provides better type safety and avoids name clashes by requiring the enum name to be scoped.

- **Example:**
    ```cpp
    enum class Color {
        Red,
        Green,
        Blue
    };

    Color color = Color::Red;
    ```

#### Use std::string instead of C-style strings

- **What:** Replace C-style strings (char arrays) with `std::string` especially `const std::string&` for parameters.
- **Why:** `std::string` provides better memory management, safety, and ease of use compared to C-style strings. Also you don't need to do a `nullptr` check. Will use const string references to avoid copying.


-   **Before:**
    ```cpp
        Node* Scene::addNode(const char* id);
    ```
-   **After:**

    ```cpp
        Node* Scene::addNode(const std::string& id);
    ```

### 2. Modern C++ Idioms

These changes make the code more concise and less error-prone.

#### Use Range-Based `for` Loops

-   **What:** Replace iterator-based loops with range-based `for` loops when iterating over an entire container.
-   **Why:** The syntax is simpler, less verbose, and eliminates common errors like incorrect begin/end conditions.

-   **Before:**
    ```cpp
    for (std::vector<int>::iterator it = my_vector.begin(); it != my_vector.end(); ++it) {
        // ...
    }
    ```
-   **After:**
    ```cpp
    for (int& value : my_vector) {
        // ...
    }
    ```

#### Use `auto` for Type Deduction

-   **What:** Use `auto` to declare variables where the type is clear from the initializer, especially for complex types like iterators or template instantiations.
-   **Why:** Reduces verbosity and makes code easier to read and refactor.

-   **Before:**
    ```cpp
    std::map<std::string, int>::const_iterator it = myMap.find("key");
    ```
-   **After:**
    ```cpp
    auto it = myMap.find("key");
    ```

#### Use std available algorithms instead deriving on our own.
-   **What:** Rely on std ready functions as muc as possible
-   **Why:** Reduces verbosity and makes code easier to read and refactor. Gives compiler chance to optimize.
 
-   **Before:**
    ```cpp
    float cpX = center.x;

    const Vector3& boxMin = box.min;
    const Vector3& boxMax = box.max;
    // Closest x value.
    if (center.x < boxMin.x)
    {
        cpX = boxMin.x;
    }
    else if (center.x > boxMax.x)
    {
        cpX = boxMax.x;
    }
    ```
-   **After:**    
    ```cpp
    float distX = std::clamp(center.x, box.min.x, box.max.x) - center.x;
    ```
 
### Use returns instead parameter out
-   **What:** Old C style functions used to update or return via reference/pointer parameters.
-   **Why:** Apply best practices and make intensions clear by simply return the requested value.
 
-   **Before:**
    ```cpp
    void Matrix::createOrthographic(float width, float height, float zNearPlane, float zFarPlane, Matrix* dst)
    ```
-   **After:**   
    ```cpp
    Matrix Matrix::createOrthographic(float width, float height, float zNearPlane, float zFarPlane)
    ```


### 3. Memory and Resource Management

This is a critical step to eliminate memory leaks and make resource ownership clear.

#### Replace Raw Pointers with Smart Pointers

-   **What:** Replace raw pointers from manual `new`/`delete` (and `SAFE_DELETE`) with standard smart pointers.
    -   `std::unique_ptr`: For exclusive ownership of a resource. This should be the default choice.
    -   `std::shared_ptr`: For shared ownership (replaces the custom `Ref` reference counting system).
-   **Why:** Automates memory management, prevents memory leaks, and clarifies resource ownership. This is the cornerstone of modern C++ resource safety (RAII).

-   **Before:**
    ```cpp
    // In Game.h
    AnimationController* _animationController;

    // In Game.cpp
    _animationController = new AnimationController();
    ...
    SAFE_DELETE(_animationController);
    ```
-   **After:**
    ```cpp
    // In Game.h
    #include <memory>
    std::unique_ptr<AnimationController> _animationController;

    // In Game.cpp
    _animationController = std::make_unique<AnimationController>();
    ...
    // No manual deletion needed!
    ```

### 4. Function and Constant Correctness

These changes leverage the type system to create more robust and potentially faster code.

#### Use `constexpr` for Compile-Time Constants

-   **What:** Replace `#define` macros for constants with `constexpr` variables.
-   **Why:** `constexpr` variables are type-safe, respect scope, and can be debugged, unlike preprocessor macros.

-   **Before:**
    ```cpp
    #define MAX_ENTITIES 1024
    ```
-   **After:**
    ```cpp
    constexpr int MAX_ENTITIES = 1024;
    ```

#### Use `const` and `noexcept` on Functions

-   **What:** Mark member functions that do not modify member data as `const`. Mark functions that are guaranteed not to throw exceptions as `noexcept`.
-   **Why:** `const` enforces correctness at compile time. `noexcept` allows the compiler to generate more optimized code by not needing to handle exception-based stack unwinding. Simple getter functions are prime candidates.

-   **Before:**
    ```cpp
    // In Game.h
    static Game* getInstance();
    bool isVsync();
    ```
-   **After:**
    ```cpp
    // In Game.h
    static Game* getInstance() noexcept;
    bool isVsync() const noexcept;
    ```
#### Use `constexpr` instead defines
-   **What:** Replace `#define` macros for constants with `constexpr` variables.
-   **Why:** `constexpr` variables are type-safe, respect scope, and can be debugged, unlike preprocessor macros.

  - -   **Before:**
    ```cpp
    #define MAX_ENTITIES 1024
    ```
  - -   **After:**
    ```cpp
    constexpr int MAX_ENTITIES = 1024;
    ```

### 5. Performance Improvements
  -   **What:** Identify and eliminate unnecessary copies of objects, especially in performance-critical code.
  -   **Why:** Copying large objects can be expensive in terms of performance. Using references or pointers can help avoid unnecessary copies.

  -  **Before:**
    ```cpp
    void processEntity(const Entity& entity); // Pass by const reference
    ```
  -  **After:**
    ```cpp
    void processEntity(const Entity& entity); // Pass by const reference
    ```
   #### Get rid of Ref class and use smart pointers instead
   - **What:** Replace the custom `Ref` class with standard smart pointers (`std::unique_ptr` or `std::shared_ptr`).
   - **Why:** Smart pointers provide automatic memory management, preventing memory leaks and dangling pointers.
   - Remove SAFE_RELEASE and SAFE_DELETE macros and use smart pointers instead.

### 6. Advanced Modern C++ Features
These changes leverage advanced C++20 features to improve code expressiveness and safety.

#### Use Concepts for Template Constraints
-   **What:** Use C++20 concepts to constrain template parameters, ensuring that they meet specific requirements.
-   **Why:** Concepts provide clearer and more meaningful error messages when template requirements are not met, improving code readability and maintainability.
  
  - **Before:**
    ```cpp
    template <class ClassType, class ParameterType> class MethodArrayBinding : public MethodBinding
    {
        typedef ParameterType (ClassType::*ValueMethod)() const;
        typedef unsigned int (ClassType::*CountMethod)() const;
    };

  - **After:**
    ```cpp
    template <typename ClassType, typename ParameterType>
    concept HasValueMethod = requires(ClassType obj) {
        { obj.valueMethod() } -> std::same_as<ParameterType>;
    };
    template <typename ClassType, typename ParameterType>
    concept HasCountMethod = requires(ClassType obj) {
        { obj.countMethod() } -> std::same_as<unsigned int>;
    };
    template <HasValueMethod<ClassType, ParameterType> ClassType, HasCountMethod<ClassType> ClassType>
    class MethodArrayBinding : public MethodBinding
    {
        typedef ParameterType (ClassType::*ValueMethod)() const;
        typedef unsigned int (ClassType::*CountMethod)() const;
    };
    ```
#### Use `std::span` for Array Views
-   **What:** Use `std::span` to represent non-owning views over contiguous sequences of elements (like arrays or vectors).
-   **Why:** `std::span` provides a safer and more expressive way to handle array-like data without losing size information, reducing the risk of buffer overflows and improving code clarity.
    
    - **Before:**
    ```cpp
    void processArray(int* arr, size_t size);
    ```
    - **After:**
    ```cpp
    #include <span>
    void processArray(std::span<int> arr);
    ```
#### Use `std::optional` for Optional Values
-   **What:** Use `std::optional` to represent values that may or may not be present, instead of using raw pointers or sentinel values.
-   **Why:** `std::optional` provides a clear and type-safe way to handle optional values, improving code readability and reducing the risk of null pointer dereferencing.

- **Before:**
    ```cpp
    int* findValue(int key); // Returns nullptr if not found
    ```
- **After:**
    ```cpp
    std::optional<int> findValue(int key); // Returns std::nullopt if not found
    ```
#### Use `std::variant` for Type-Safe Unions
-   **What:** Use `std::variant` to represent a value that can be one of several types, instead of using unions or `void*`.
-   **Why:** `std::variant` provides type safety and eliminates the need for manual type checking and casting.

- **Before:**
    ```cpp
    union Data {
        int intValue;
        float floatValue;
        char* stringValue;
    };
    ```
- **After:**
    ```cpp
    using Data = std::variant<int, float, std::string>;
    ```

#### Use `std::ranges` for Range-Based Algorithms
-   **What:** Use `std::ranges` to simplify and enhance range-based algorithms.
-   **Why:** `std::ranges` provides a more expressive and safer way to work with ranges, reducing the risk of errors and improving code clarity.
-   **Before:**
    ```cpp
    for (size_t i = 0, count = _variables->size(); i < count; ++i)
    {
        Property& prop = (*_variables)[i];
        if (prop.name == name)
        return prop.value.c_str();
    }
    ```
-   **After:**
    ```cpp
    auto it = std::ranges::find_if(*_variables, [name](const Property& property) {
        return property.name == name;
    });

    if (it != _variables->end())
        return it->value.c_str();
    ```

-   **Before:**
    ```cpp
    for (; iter != _propertiesFromFile.end(); ++iter)
    {
      SAFE_DELETE(iter->second);
    }
    ```
-   **After:**
  - Note that erasing items from the containers, while iterating requires different approach
    ```cpp
    std::erase_if(_propertiesFromFile, [](auto& pair) {
      pair.second->reset();
      return true; // erase each of them
      });
    ```


### 7. Code Cleanup and Refactoring
-   **What:** Regularly review and refactor code to improve readability, maintainability, and performance.
-   **Why:** Clean code is easier to understand, modify, and extend. Refactoring can also help eliminate technical debt and improve performance.
-   **Before:**
    ```cpp
    // Before
    for (int i = 0; i < 10; i++) {
        processEntity(entities[i]);
    }
    ```
-   **After:**
    ```cpp
    for (const auto& entity : entities)
        processEntity(entity);
    ```

### 8. Testing and Validation
-   **What:** Implement comprehensive testing strategies to ensure code correctness and reliability.
-   **Why:** Testing helps identify bugs and issues early in the development process, reducing the cost of fixing them later.

### Add a good error handling strategy
-   **What:** Implement a consistent error handling strategy using exceptions or error codes.
-   **Why:** Consistent error handling improves code reliability and makes it easier to diagnose and fix issues.

-   **Note:** Don't use throw catch for control flow, only for exceptional situations.


## Other things to consider
- [ ] Use `std::function and std::bind` instead old function pointers
- [ ] Use `std::span` for arrays where available
- [ ] Utilize `std::ranges` and `views` where makes sense
- [ ] Prefer `std::optional` for error recovery rather than `nullptr` checks
- [ ] Use c++ `modules` if it makes sense
- [ ] Use `lambdas` for repeating steps within a function
- [ ] Use `std::variant` for unions or to handle multiple types
- [ ] Prefer `using =` statements instead `typedef`'s
- [X] Use `std::filesystem` for IO operations
- [-] Make properties logic clearer and maintainable
- [ ] Investigate `Entity Component Systems (ECS)` instead deep inheritances.
- [ ] Adding unit tests initially for mathematical computations.
- [ ] Add a reliable logging system instead error or warning macros.
- [-] Enabling a consistent clang-tidy rules.
- [-] Instead regular loops utilize std ready data iterations.
- [-] using size_t for indexes?
- [ ] To be continued...