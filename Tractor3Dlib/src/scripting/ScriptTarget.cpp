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
#include "pch.h"

#include "scripting/ScriptTarget.h"

#include "scripting/ScriptController.h"

namespace tractor
{

// TODO: Handle reloading scripts and then case where EventRegistries have events added AFTER a
// script has been loaded (i.e. need to reload callbacks for the script).

extern void splitURL(const std::string& url, std::string* file, std::string* id);

const std::string& ScriptTarget::Event::getName() const noexcept { return name.c_str(); }

const std::string& ScriptTarget::Event::getArgs() const noexcept { return args; }

ScriptTarget::EventRegistry::EventRegistry() {}

ScriptTarget::EventRegistry::~EventRegistry()
{
    for (size_t i = 0, count = _events.size(); i < count; ++i)
    {
        delete _events[i];
    }
}

const ScriptTarget::Event* ScriptTarget::EventRegistry::addEvent(const char* name, const char* args)
{
    assert(name);

    auto& evt = _events.emplace_back(new Event());
    evt->name = name;
    evt->args = args ? args : "";

    return evt;
}

unsigned int ScriptTarget::EventRegistry::getEventCount() const noexcept { return _events.size(); }

const ScriptTarget::Event* ScriptTarget::EventRegistry::getEvent(unsigned int index) const
{
    assert(index < _events.size());

    return _events[index];
}

const ScriptTarget::Event* ScriptTarget::EventRegistry::getEvent(const char* name) const
{
    assert(name);

    for (size_t i = 0, count = _events.size(); i < count; ++i)
    {
        if (_events[i]->name == name) return _events[i];
    }

    return nullptr;
}

ScriptTarget::ScriptTarget()
    : _scriptRegistries(nullptr), _scripts(nullptr), _scriptCallbacks(nullptr)
{
}

ScriptTarget::~ScriptTarget()
{
    // Free callbacks
    delete _scriptCallbacks;
    _scriptCallbacks = nullptr;

    // Free scripts
    ScriptEntry* se = _scripts;
    while (se)
    {
        ScriptEntry* tmp = se;
        se = se->next;

        // shared_ptr will release automatically when ScriptEntry is deleted
        delete tmp;
    }

    // Free registry entries
    RegistryEntry* re = _scriptRegistries;
    while (re)
    {
        RegistryEntry* tmp = re;
        re = re->next;

        // Don't delete the actual EventRegistry, since it's shared by all
        // ScriptTargets of the same type
        delete tmp;
    }
}

void ScriptTarget::registerEvents(EventRegistry* registry)
{
    assert(registry);

    // Attach the registry
    RegistryEntry* re = new RegistryEntry(registry);
    if (_scriptRegistries)
    {
        RegistryEntry* last = _scriptRegistries;
        while (last->next)
            last = last->next;
        last->next = re;
        re->prev = last;
    }
    else
    {
        _scriptRegistries = re;
    }
}

ScriptPtr ScriptTarget::addScript(const std::string& path)
{
    ScriptController* sc = Game::getInstance()->getScriptController();

    // Load the script
    ScriptPtr script = sc->loadScript(path, Script::PROTECTED);
    if (!script) return nullptr;

    // Attach the script
    ScriptEntry* se = new ScriptEntry(script);
    if (_scripts)
    {
        ScriptEntry* last = _scripts;
        while (last->next)
            last = last->next;
        last->next = se;
        se->prev = last;
    }
    else
    {
        _scripts = se;
    }

    // Inspect the loaded script for event functions that are supported by this ScriptTarget.
    // TODO: We'll need to re-load eventCallbacks when EventRegistries change for this ScriptObject.
    RegistryEntry* re = _scriptRegistries;
    while (re)
    {
        std::vector<Event*>& events = re->registry->_events;
        for (size_t i = 0, count = events.size(); i < count; ++i)
        {
            const Event* event = events[i];
            if (sc->functionExists(event->name.c_str(), script.get()))
            {
                if (!_scriptCallbacks)
                    _scriptCallbacks = new std::map<const Event*, std::vector<CallbackFunction>>();
                (*_scriptCallbacks)[event].emplace_back(CallbackFunction(script, event->name.c_str()));
            }
        }
        re = re->next;
    }

    // Automatically call the 'attached' event if it is defined within the script
    if (sc->functionExists("attached", script.get()))
    {
        sc->executeFunction<void>(script.get(), "attached", "<ScriptTarget>", nullptr, (void*)this);
    }

    return script;
}

bool ScriptTarget::removeScript(const std::string& path)
{
    ScriptEntry* se = _scripts;
    while (se)
    {
        if (se->script->getPath() == path && se->script->getScope() == Script::PROTECTED)
        {
            removeScript(se);
            return true;
        }
        se = se->next;
    }

    return false;
}

void ScriptTarget::removeScript(ScriptEntry* se)
{
    assert(se);

    // Link out this ScriptEntry
    if (se->prev) se->prev->next = se->next;
    if (se->next) se->next->prev = se->prev;
    if (_scripts == se) _scripts = se->next;

    ScriptPtr script = se->script;

    // Delete the ScriptEntry
    delete se;

    // Erase any callback functions registered for this script
    if (_scriptCallbacks)
    {
        std::ranges::for_each(*_scriptCallbacks,
                              [&script](auto& pair)
                              {
                                  auto& callbacks = pair.second;
                                  std::erase_if(callbacks,
                                                [&script](const CallbackFunction& callback)
                                                { return callback.script == script; });
                              });
    }

    // script shared_ptr will release automatically when going out of scope
}

void ScriptTarget::addScriptCallback(const Event* event, const char* function)
{
    assert(event);
    assert(function);

    // Parse the script name (if it exists) and function out
    std::string scriptPath, func;
    splitURL(function, &scriptPath, &func);
    if (func.length() == 0)
    {
        // The url doesn't reference a script, only a function
        func = scriptPath;
        scriptPath = "";
    }

    // Have we already loaded this global script?
    bool loaded = true;
    ScriptPtr script = nullptr;
    if (!scriptPath.empty())
    {
        loaded = false;
        ScriptEntry* se = _scripts;
        while (se)
        {
            if (scriptPath == se->script->getPath() && se->script->getScope() == Script::GLOBAL)
            {
                // Script is already loaded
                script = se->script;
                loaded = true;
                break;
            }
            se = se->next;
        }
    }

    if (!loaded)
    {
        // The specified global script is not yet loaded, so do so
        script = Game::getInstance()->getScriptController()->loadScript(scriptPath, Script::GLOBAL);
        if (script)
        {
            loaded = true;
            ScriptEntry* se = new ScriptEntry(script);
            if (_scripts)
            {
                ScriptEntry* last = _scripts;
                while (last->next)
                    last = last->next;
                last->next = se;
                se->prev = last;
            }
            else
            {
                _scripts = se;
            }
        }
        else
        {
            GP_WARN("Failed to load script '%s' for script target while registering for function: "
                    "%s",
                    scriptPath.c_str(),
                    function);
        }
    }

    if (loaded)
    {
        // Store the callback
        if (!_scriptCallbacks)
            _scriptCallbacks = new std::map<const Event*, std::vector<CallbackFunction>>();
        (*_scriptCallbacks)[event].emplace_back(CallbackFunction(script, func.c_str()));
    }
}

void ScriptTarget::removeScriptCallback(const Event* event, const char* function)
{
    // Parse the script name (if it exists) and function out
    std::string scriptPath, func;
    splitURL(function, &scriptPath, &func);
    if (func.length() == 0)
    {
        // The url doesn't reference a script, only a function
        func = scriptPath;
        scriptPath = "";
    }

    // Find the script entry for this callback
    ScriptEntry* scriptEntry = nullptr;
    if (!scriptPath.empty())
    {
        for (auto& se : *_scripts)
        {
            if (scriptPath == se.script->getPath() && se.script->getScope() == Script::GLOBAL)
            {
                scriptEntry = &se;
                break;
            }
        }
    }
    ScriptPtr script = scriptEntry ? scriptEntry->script : nullptr;

    // Remove any registered callback functions that match the specified one
    int removedCallbacks = 0;
    int totalCallbacks = 0;
    if (_scriptCallbacks)
    {
        for (auto& [registeredEvent, callbacks] : *_scriptCallbacks)
        {
            // Check if this event matches the specified event for selective removal
            bool forEvent = registeredEvent == event;

            // Count callbacks matching the script before potentially erasing them
            totalCallbacks += std::ranges::count_if(callbacks,
                                                    [&](const CallbackFunction& cb)
                                                    { return cb.script == script; });

            // Remove callbacks that match both the script and function for the given event
            removedCallbacks +=
                std::erase_if(callbacks,
                              [&](const CallbackFunction& cb)
                              { return cb.script == script && (forEvent && cb.function == func); });
        }
    }

    // Cleanup the script if there are no remaining callbacks for it
    if (scriptEntry && (totalCallbacks - removedCallbacks) <= 0)
    {
        removeScript(scriptEntry);
    }
}

void ScriptTarget::clearScripts()
{
    while (_scripts)
    {
        removeScript(_scripts);
    }
}

bool ScriptTarget::hasScriptListener(const char* eventName) const
{
    const Event* event = getScriptEvent(eventName);
    return event ? hasScriptListener(event) : false;
}

const ScriptTarget::Event* ScriptTarget::getScriptEvent(const char* eventName) const
{
    assert(eventName);

    // Lookup the event for this name
    const Event* event = nullptr;
    RegistryEntry* re = _scriptRegistries;
    while (re)
    {
        if ((event = re->registry->getEvent(eventName)) != nullptr) break;
        re = re->next;
    }

    return event;
}

bool ScriptTarget::hasScriptListener(const Event* event) const
{
    assert(event);

    if (_scriptCallbacks)
    {
        std::map<const Event*, std::vector<CallbackFunction>>::iterator itr =
            _scriptCallbacks->find(event);
        if (itr != _scriptCallbacks->end())
        {
            return !itr->second.empty();
        }
    }

    return false;
}

template <> void ScriptTarget::fireScriptEvent<void>(const Event* event, ...)
{
    assert(event);

    if (!_scriptCallbacks) return; // no registered callbacks

    va_list list;
    va_start(list, event);

    // Lookup registered callbacks for this event and fire them
    std::map<const Event*, std::vector<CallbackFunction>>::iterator itr =
        _scriptCallbacks->find(event);
    if (itr != _scriptCallbacks->end())
    {
        ScriptController* sc = Game::getInstance()->getScriptController();
        std::vector<CallbackFunction>& callbacks = itr->second;
        for (size_t i = 0, count = callbacks.size(); i < count; ++i)
        {
            CallbackFunction& cb = callbacks[i];
            sc->executeFunction<void>(cb.script.get(),
                                      cb.function.c_str(),
                                      event->args.c_str(),
                                      nullptr,
                                      &list);
        }
    }

    va_end(list);
}

template <> bool ScriptTarget::fireScriptEvent<bool>(const Event* event, ...)
{
    assert(event);

    if (!_scriptCallbacks) return false; // no registered callbacks

    va_list list;
    va_start(list, event);

    // Lookup registered callbacks for this event and fire them
    std::map<const Event*, std::vector<CallbackFunction>>::iterator itr =
        _scriptCallbacks->find(event);
    if (itr != _scriptCallbacks->end())
    {
        ScriptController* sc = Game::getInstance()->getScriptController();
        std::vector<CallbackFunction>& callbacks = itr->second;
        for (size_t i = 0, count = callbacks.size(); i < count; ++i)
        {
            CallbackFunction& cb = callbacks[i];
            bool result = false;
            if (sc->executeFunction<bool>(cb.script.get(),
                                          cb.function.c_str(),
                                          event->args.c_str(),
                                          &result,
                                          &list)
                && result)
            {
                // Handled, break out early
                va_end(list);
                return true;
            }
        }
    }

    va_end(list);

    return false;
}

} // namespace tractor
