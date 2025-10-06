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

#include "ui/Theme.h"

namespace tractor
{

class Properties;
class Control;

/**
 * Defines a factory for creating core controls and registered custom controls.
 *
 * @script{ignore}
 */
class ControlFactory
{
    friend class Game;
    friend class Container;

  public:
    /**
     * The activator interface for controls that are created.
     */
    typedef Control* (*ControlActivator)(Theme::Style*, Properties*);

    /**
     * Gets the single instance of the control factory used to create controls and
     * register/unregister custom controls.
     *
     * @return The instance of the ControlFactory.
     */
    static ControlFactory* getInstance();

    /**
     * Registers a custom control and specify the activator.
     *
     * @param typeName The name of the custom control type to register.
     * @param activator The activator for applying the style, properties and theme to the control.
     *
     * @return true if the control was successfully registered.
     */
    bool registerCustomControl(const std::string& typeName, ControlActivator activator);

    /**
     * Unregisters a custom control and specify the activator.
     *
     * @param typeName The name of the custom control type to unregister.
     */
    void unregisterCustomControl(const std::string& typeName);

  private:
    /**
     * Constructor.
     */
    ControlFactory();

    /**
     * Constructor.
     */
    ControlFactory(const ControlFactory& copy) = default;

    /**
     * Destructor.
     */
    ~ControlFactory() = default;

    /**
     * Cleans up resources allocated by the ControlFactory.
     */
    static void finalize();

    /**
     * Assignment operator
     */
    ControlFactory& operator=(const ControlFactory&) = delete;

    /**
     * Creates a controls from the set of core and custom controls registered.
     *
     * @param typeName The type of the control to create.
     * @param style The style to apply to the control.
     * @param properties A Properties object describing the control (optional).
     * @return The new control.
     */
    Control* createControl(const std::string& typeName,
                           Theme::Style* style,
                           Properties* properties = nullptr);

    /**
     * Registers the standard (built-in) controls
     */
    void registerStandardControls();

    std::map<std::string, ControlActivator> _registeredControls;
};

} // namespace tractor
