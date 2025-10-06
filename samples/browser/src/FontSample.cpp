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
#include "FontSample.h"

#include "SamplesGame.h"

#if defined(ADD_SAMPLE)
ADD_SAMPLE("Graphics", "Font", FontSample, 7);
#endif

#define FONT_COUNT 5

std::string _fontNames[] = { "arial", "arial-dist.field", "badaboom", "fishfingers", "neuropol" };

std::string _fontFiles[] = { "res/ui/arial.gpb",
                             "res/common/fonts/arial-distance.gpb",
                             "res/common/fonts/badaboom.gpb",
                             "res/common/fonts/fishfingers.gpb",
                             "res/common/fonts/neuropol.gpb" };

FontSample::FontSample()
    : _form(nullptr), _stateBlock(nullptr), _size(18), _wrap(true), _ignoreClip(false),
      _useViewport(true), _rightToLeft(false), _simple(false), _alignment(Font::ALIGN_LEFT),
      _fontsCount(FONT_COUNT), _fontIndex(0), _font(nullptr), _viewport(250, 100, 512, 200)
{
}

void FontSample::finalize()
{
    SAFE_RELEASE(_stateBlock);

    for (size_t i = 0; i < _fonts.size(); i++)
    {
        SAFE_RELEASE(_fonts[i]);
    }

    SAFE_RELEASE(_form);
}

void FontSample::initialize()
{
    // Create our render state block that will be reused across all materials
    _stateBlock = RenderState::StateBlock::create();
    _stateBlock->setCullFace(true);
    _stateBlock->setDepthTest(true);
    _stateBlock->setBlend(true);
    _stateBlock->setBlendSrc(RenderState::BLEND_SRC_ALPHA);
    _stateBlock->setBlendDst(RenderState::BLEND_ONE_MINUS_SRC_ALPHA);

    for (size_t i = 0; i < _fontsCount; i++)
    {
        _fonts.emplace_back(Font::create(_fontFiles[i]));
    }
    _font = _fonts[0];

    _sampleString = std::string("Lorem ipsum dolor sit amet, \n"
                                "consectetur adipisicing elit, sed do eiusmod tempor incididunt ut "
                                "labore et dolore magna aliqua.\n"
                                "Ut enim ad minim veniam, quis nostrud exercitation ullamco "
                                "laboris nisi ut aliquip ex ea commodo consequat.\n"
                                "Duis aute irure dolor in reprehenderit in voluptate velit esse "
                                "cillum dolore eu fugiat nulla pariatur.\n"
                                "Excepteur sint occaecat cupidatat non proident, sunt in culpa qui "
                                "officia deserunt mollit anim id est laborum.");

    // Create and listen to form.
    _form = Form::create("res/common/text.form");
    static_cast<Button*>(_form->getControl("fontButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("wrapButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("clipRectButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("reverseButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("switchClipRegionButton"))
        ->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("simpleAdvancedButton"))
        ->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("smallerButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("biggerButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("topLeftButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("topCenterButton"))
        ->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("topRightButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("centerLeftButton"))
        ->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("centerButton"))->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("centerRightButton"))
        ->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("bottomLeftButton"))
        ->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("bottomCenterButton"))
        ->addListener(this, Control::Listener::CLICK);
    static_cast<Button*>(_form->getControl("bottomRightButton"))
        ->addListener(this, Control::Listener::CLICK);
}

void FontSample::update(float elapsedTime) {}

void FontSample::render(float elapsedTime)
{
    // Clear the screen.
    clear(CLEAR_COLOR_DEPTH, Vector4(0, 0, 0, 1), 1.0f, 0);

    // Draw the frame rate.
    char fps[5];
    sprintf(fps, "%u", getFrameRate());

    _fonts[0]->start();

    _fonts[0]->drawText(fps, 245, 5, Vector4(0, 0.5f, 1, 1), _size);

    if (_font != _fonts[0]) _font->start();

    if (_simple)
    {
        // Sample simple versions of measureText, drawText.
        unsigned int w, h;
        _font->measureText(_sampleString, _size, &w, &h);
        _font->drawText(_sampleString,
                        _viewport.x,
                        _viewport.y,
                        Vector4::fromColor(0xff0000ff),
                        _size,
                        _rightToLeft);

        _font->drawText("'", _viewport.x, _viewport.y, Vector4::fromColor(0x00ff00ff), _size);
        _font->drawText(".", _viewport.x, _viewport.y + h, Vector4::fromColor(0x00ff00ff), _size);
        _font->drawText("'", _viewport.x + w, _viewport.y, Vector4::fromColor(0x00ff00ff), _size);
        _font->drawText(".", _viewport.x + w, _viewport.y + h, Vector4::fromColor(0x00ff00ff), _size);
    }
    else
    {
        // Sample viewport versions.
        tractor::Rectangle area;
        _font->measureText(_sampleString, _viewport, _size, &area, _alignment, _wrap, _ignoreClip);
        _font->drawText(_sampleString,
                        _useViewport ? _viewport : area,
                        Vector4::fromColor(0xffffffff),
                        _size,
                        _alignment,
                        _wrap,
                        _rightToLeft);

        _font->drawText("'", _viewport.x, _viewport.y, Vector4::fromColor(0x00ff00ff), _size);
        _font->drawText(".",
                        _viewport.x,
                        _viewport.y + _viewport.height,
                        Vector4::fromColor(0x00ff00ff),
                        _size);
        _font->drawText("'",
                        _viewport.x + _viewport.width,
                        _viewport.y,
                        Vector4::fromColor(0x00ff00ff),
                        _size);
        _font->drawText(".",
                        _viewport.x + _viewport.width,
                        _viewport.y + _viewport.height,
                        Vector4::fromColor(0x00ff00ff),
                        _size);

        _font->drawText("'", area.x, area.y, Vector4::fromColor(0x0000ffff), _size);
        _font->drawText(".", area.x, area.y + area.height, Vector4::fromColor(0x0000ffff), _size);
        _font->drawText("'", area.x + area.width, area.y, Vector4::fromColor(0x0000ffff), _size);
        _font->drawText(".",
                        area.x + area.width,
                        area.y + area.height,
                        Vector4::fromColor(0x0000ffff),
                        _size);
    }

    if (_font != _fonts[0])
    {
        _font->finish();
    }
    _fonts[0]->finish();

    _form->draw();
}

void FontSample::touchEvent(Touch::TouchEvent event, int x, int y, unsigned int contactIndex)
{
    _viewport.width = x - _viewport.x;
    _viewport.height = y - _viewport.y;
}

void FontSample::controlEvent(Control* control, EventType evt)
{
    const std::string& id = control->getId();

    if (id == "fontButton")
    {
        _fontIndex++;
        if (_fontIndex >= _fontsCount)
        {
            _fontIndex = 0;
        }
        _font = _fonts[_fontIndex];
        std::string s = "Font (" + _fontNames[_fontIndex] + ")";
        static_cast<Button*>(control)->setText(s);
    }
    else if (id == "wrapButton")
    {
        _wrap = !_wrap;
        Button* wrapButton = static_cast<Button*>(control);
        if (_wrap)
            wrapButton->setText("Word Wrap (On)");
        else
            wrapButton->setText("Word Wrap (Off)");
    }
    else if (id == "clipRectButton")
    {
        _ignoreClip = !_ignoreClip;
        Button* clipRectButton = static_cast<Button*>(control);
        if (_ignoreClip)
            clipRectButton->setText("Clipping (Off)");
        else
            clipRectButton->setText("Clipping (On)");
    }
    else if (id == "reverseButton")
    {
        _rightToLeft = !_rightToLeft;
        Button* reverseButton = static_cast<Button*>(control);
        if (_rightToLeft)
            reverseButton->setText("Reverse Text (On)");
        else
            reverseButton->setText("Reverse Text (Off)");
    }
    else if (id == "switchClipRegionButton")
    {
        _useViewport = !_useViewport;
        Button* switchClipButton = static_cast<Button*>(control);
        if (_useViewport)
            switchClipButton->setText("Clip Regions (Viewport)");
        else
            switchClipButton->setText("Clip Regions (Text Area)");
    }
    else if (id == "simpleAdvancedButton")
    {
        _simple = !_simple;
        Button* simpleAdvancedButton = static_cast<Button*>(control);
        if (_simple)
            simpleAdvancedButton->setText("Font API (Simple)");
        else
            simpleAdvancedButton->setText("Font API (Advanced)");
    }
    else if (id == "smallerButton")
    {
        if (_size > 8)
        {
            _size -= 2;
            Label* sizeLabel = static_cast<Label*>(_form->getControl("sizeLabel"));
            char s[20];
            sprintf(s, "Size (%u)", _size);
            sizeLabel->setText(s);
        }
    }
    else if (id == "biggerButton")
    {
        _size += 2;
        Label* sizeLabel = static_cast<Label*>(_form->getControl("sizeLabel"));
        char s[20];
        sprintf(s, "Size (%u)", _size);
        sizeLabel->setText(s);
    }
    else if (id == "topLeftButton")
    {
        _alignment = Font::ALIGN_TOP_LEFT;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Top-Left)");
    }
    else if (id == "topCenterButton")
    {
        _alignment = Font::ALIGN_TOP_HCENTER;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Top-Center)");
    }
    else if (id == "topRightButton")
    {
        _alignment = Font::ALIGN_TOP_RIGHT;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Top-Right)");
    }
    else if (id == "centerLeftButton")
    {
        _alignment = Font::ALIGN_VCENTER_LEFT;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Center-Left)");
    }
    else if (id == "centerButton")
    {
        _alignment = Font::ALIGN_VCENTER_HCENTER;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Center)");
    }
    else if (id == "centerRightButton")
    {
        _alignment = Font::ALIGN_VCENTER_RIGHT;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Center-Right)");
    }
    else if (id == "bottomLeftButton")
    {
        _alignment = Font::ALIGN_BOTTOM_LEFT;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Bottom-Left)");
    }
    else if (id == "bottomCenterButton")
    {
        _alignment = Font::ALIGN_BOTTOM_HCENTER;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Bottom-Center)");
    }
    else if (id == "bottomRightButton")
    {
        _alignment = Font::ALIGN_BOTTOM_RIGHT;
        Label* alignmentLabel = static_cast<Label*>(_form->getControl("alignmentLabel"));
        alignmentLabel->setText("Align (Bottom-Right)");
    }
}
