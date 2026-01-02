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

#include "math/Vector3.h"
#include "math/Vector4.h"
#include "scene/Properties.h"

namespace tractor
{

/**
 * Utility class for parsing common property patterns.
 * 
 * This class consolidates common property reading patterns that were
 * duplicated across multiple classes (AudioSource, Light, Sprite, Text, TileSet, etc.).
 * It follows the DRY principle by centralizing color and other common parsing logic.
 */
class PropertyUtils
{
public:
    /**
     * Reads a color value from properties, handling Vector3, Vector4, and string formats.
     *
     * @param properties The properties object to read from.
     * @param name The name of the color property.
     * @param out The Vector4 to store the result (alpha defaults to 1.0 for RGB values).
     * @return true if the color was successfully read, false otherwise.
     */
    static bool getColor(Properties* properties, const std::string& name, Vector4* out);

    /**
     * Reads a color value from properties as Vector3 (RGB only).
     *
     * @param properties The properties object to read from.
     * @param name The name of the color property.
     * @param out The Vector3 to store the result.
     * @return true if the color was successfully read, false otherwise.
     */
    static bool getColor(Properties* properties, const std::string& name, Vector3* out);

    /**
     * Reads a path from properties, resolving relative paths.
     *
     * @param properties The properties object to read from.
     * @param name The name of the path property.
     * @param path Output parameter for the resolved path.
     * @return true if the path was found and resolved, false otherwise.
     */
    static bool getPath(Properties* properties, const std::string& name, std::string& path);

    /**
     * Reads an optional float property with a default value.
     *
     * @param properties The properties object to read from.
     * @param name The name of the property.
     * @param defaultValue The default value if property doesn't exist.
     * @return The property value or defaultValue if not found.
     */
    static float getFloat(Properties* properties, const std::string& name, float defaultValue);

    /**
     * Reads an optional int property with a default value.
     *
     * @param properties The properties object to read from.
     * @param name The name of the property.
     * @param defaultValue The default value if property doesn't exist.
     * @return The property value or defaultValue if not found.
     */
    static int getInt(Properties* properties, const std::string& name, int defaultValue);

    /**
     * Reads an optional bool property with a default value.
     *
     * @param properties The properties object to read from.
     * @param name The name of the property.
     * @param defaultValue The default value if property doesn't exist.
     * @return The property value or defaultValue if not found.
     */
    static bool getBool(Properties* properties, const std::string& name, bool defaultValue);

    /**
     * Validates that a Properties object is non-null and has the expected namespace.
     *
     * @param properties The properties object to validate.
     * @param expectedNamespace The expected namespace name.
     * @param errorContext Context string for error messages (e.g., "audio source").
     * @return true if valid, false otherwise (with error logged).
     */
    static bool validateNamespace(Properties* properties, 
                                  const std::string& expectedNamespace,
                                  const std::string& errorContext);

private:
    PropertyUtils() = delete; // Static utility class
};

} // namespace tractor
