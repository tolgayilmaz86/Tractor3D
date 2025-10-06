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

#include "Sample.h"
#include "tractor.h"

using namespace tractor;

/**
 * Sample testing the font with various functionality.
 */
class FontSample : public Sample, public Control::Listener
{
  public:
    FontSample();

  protected:
    void initialize();
    void finalize();
    void update(float elapsedTime);
    void render(float elapsedTime);
    void touchEvent(Touch::TouchEvent event, int x, int y, unsigned int contactIndex);
    void controlEvent(Control* control, EventType evt);

  private:
    void renderToTexture();
    void buildQuad(Texture* texture);

    Form* _form;
    RenderState::StateBlock* _stateBlock;
    unsigned int _size;
    bool _wrap;
    bool _ignoreClip;
    bool _useViewport;
    bool _rightToLeft;
    bool _simple;
    Font::Justify _alignment;
    std::vector<Font*> _fonts;
    unsigned int _fontsCount;
    unsigned int _fontIndex;
    Font* _font;
    Rectangle _viewport;
    std::string _sampleString;
};
