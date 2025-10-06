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

#include <scripting/Script.h>
#include <scripting/ScriptController.h>

namespace tractor
{

//----------------------------------------------------------------------------
Script::~Script() { Game::getInstance()->getScriptController()->unloadScript(this); }

bool Script::functionExists(const char* name) const
//----------------------------------------------------------------------------
{
    return Game::getInstance()->getScriptController()->functionExists(name, this);
}

//----------------------------------------------------------------------------
bool Script::reload()
{
    ScriptController* sc = Game::getInstance()->getScriptController();

    // First unload our current script
    sc->unloadScript(this);

    // Now attempt to reload the script
    return Game::getInstance()->getScriptController()->loadScript(this);
}

} // namespace tractor
