#include "pch.h"

#include "ui/Layout.h"

#include "framework/Game.h"
#include "ui/Container.h"
#include "ui/Control.h"

namespace tractor
{

bool Layout::touchEvent(Touch::TouchEvent evt, int x, int y, unsigned int contactIndex)
{
    return false;
}

} // namespace tractor