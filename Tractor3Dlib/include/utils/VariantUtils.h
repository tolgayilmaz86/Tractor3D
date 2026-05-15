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

#include <variant>

namespace tractor
{

/**
 * @brief Helper for creating overloaded visitor lambdas for std::visit.
 * 
 * This template enables the visitor pattern with std::variant by combining
 * multiple lambda functions into a single callable object.
 * 
 * Example usage:
 * @code
 * std::variant<int, float, std::string> value = 42;
 * 
 * std::visit(overloaded{
 *     [](int i)                { std::cout << "int: " << i; },
 *     [](float f)              { std::cout << "float: " << f; },
 *     [](const std::string& s) { std::cout << "string: " << s; }
 * }, value);
 * @endcode
 * 
 * @tparam Ts The types of the callable objects (typically lambdas).
 */
template<class... Ts>
struct overloaded : Ts...
{
    using Ts::operator()...;
};

// Deduction guide for C++17
template<class... Ts>
overloaded(Ts...) -> overloaded<Ts...>;

/**
 * @brief Visitor helper for operations that return the same type from all alternatives.
 * 
 * Use this when all variant alternatives have a common member or operation.
 * 
 * Example usage:
 * @code
 * // All light types have a color member
 * const Vector3& color = std::visit([](const auto& data) -> const Vector3& {
 *     return data.color;
 * }, lightData);
 * @endcode
 */

/**
 * @brief Type trait to check if a type is a std::variant.
 */
template<typename T>
struct is_variant : std::false_type {};

template<typename... Ts>
struct is_variant<std::variant<Ts...>> : std::true_type {};

template<typename T>
inline constexpr bool is_variant_v = is_variant<T>::value;

/**
 * @brief Helper to get the index of a type in a variant.
 * 
 * @tparam T The type to find.
 * @tparam V The variant type.
 */
template<typename T, typename V>
struct variant_index;

template<typename T, typename... Ts>
struct variant_index<T, std::variant<Ts...>>
    : std::integral_constant<size_t, 
        []<size_t... Is>(std::index_sequence<Is...>) {
            return ((std::is_same_v<T, Ts> ? Is : 0) + ...);
        }(std::index_sequence_for<Ts...>{})> {};

template<typename T, typename V>
inline constexpr size_t variant_index_v = variant_index<T, V>::value;

} // namespace tractor
