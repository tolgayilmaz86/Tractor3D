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
#include "AudioSample.h"

#if defined(ADD_SAMPLE)
ADD_SAMPLE("Media", "Audio Player", AudioSample, 2);
#endif

AudioSample::AudioSample()
    : _formBackground(nullptr), _formEngine(nullptr), _formBraking(nullptr),
      _audioBackground(nullptr), _audioEngine(nullptr), _audioBraking(nullptr)
{
}

void AudioSample::initialize()
{
    _formBackground = Form::create("res/common/audio/background.form");

    const char* buttons[] = { "playButton", "pauseButton", "resumeButton", "rewindButton", "stopButton" };
    for (size_t i = 0; i < sizeof(buttons) / sizeof(uintptr_t); i++)
    {
        Button* button = static_cast<Button*>(_formBackground->getControl(buttons[i]));
        button->addListener(this, Control::Listener::RELEASE);
    }

    // Create the audio source here, and feed the values into the UI controls.
    _audioBackground = AudioSource::create("res/common/audio/sample.audio#backgroundTrack");
    _audioBackground->play();

    CheckBox* checkBox = static_cast<CheckBox*>(_formBackground->getControl("loopCheckBox"));
    checkBox->setChecked(_audioBackground->isLooped());
    checkBox->addListener(this, Control::Listener::VALUE_CHANGED);

    Slider* slider = static_cast<Slider*>(_formBackground->getControl("gainSlider"));
    _audioBackground->setGain(slider->getValue());
    slider->addListener(this, Control::Listener::VALUE_CHANGED);

    slider = static_cast<Slider*>(_formBackground->getControl("pitchSlider"));
    slider->setValue(_audioBackground->getPitch());
    slider->addListener(this, Control::Listener::VALUE_CHANGED);

    _formBraking = Form::create("res/common/audio/braking.form");

    Button* button = static_cast<Button*>(_formBraking->getControl("playBrakingButton"));
    button->addListener(this, Control::Listener::RELEASE);

    button = static_cast<Button*>(_formBraking->getControl("stopBrakingButton"));
    button->addListener(this, Control::Listener::RELEASE);

    _formEngine = Form::create("res/common/audio/engine.form");

    button = static_cast<Button*>(_formEngine->getControl("playEngineButton"));
    button->addListener(this, Control::Listener::RELEASE);

    button = static_cast<Button*>(_formEngine->getControl("stopEngineButton"));
    button->addListener(this, Control::Listener::RELEASE);

    _audioEngine = AudioSource::create("res/common/audio/sample.audio#engine");

    checkBox = static_cast<CheckBox*>(_formEngine->getControl("loopEngineCheckBox"));
    checkBox->setChecked(_audioEngine->isLooped());
    checkBox->addListener(this, Control::Listener::VALUE_CHANGED);

    slider = static_cast<Slider*>(_formEngine->getControl("gainEngineSlider"));
    slider->setValue(_audioEngine->getGain());
    slider->addListener(this, Control::Listener::VALUE_CHANGED);

    slider = static_cast<Slider*>(_formEngine->getControl("pitchEngineSlider"));
    slider->setValue(_audioEngine->getPitch());
    slider->addListener(this, Control::Listener::VALUE_CHANGED);

    _audioBraking = AudioSource::create("res/common/audio/sample.audio#braking");

    checkBox = static_cast<CheckBox*>(_formBraking->getControl("loopBrakingCheckBox"));
    checkBox->setChecked(_audioBraking->isLooped());
    checkBox->addListener(this, Control::Listener::VALUE_CHANGED);

    slider = static_cast<Slider*>(_formBraking->getControl("gainBrakingSlider"));
    slider->setValue(_audioBraking->getGain());
    slider->addListener(this, Control::Listener::VALUE_CHANGED);

    slider = static_cast<Slider*>(_formBraking->getControl("pitchBrakingSlider"));
    slider->setValue(_audioBraking->getPitch());
    slider->addListener(this, Control::Listener::VALUE_CHANGED);
}

void AudioSample::finalize()
{
    SAFE_RELEASE(_audioBraking);
    SAFE_RELEASE(_audioEngine);
    SAFE_RELEASE(_audioBackground);
    SAFE_RELEASE(_formBraking);
    SAFE_RELEASE(_formEngine);
    SAFE_RELEASE(_formBackground);
}

void AudioSample::update(float elapsedTime)
{
    if (_formBackground) _formBackground->update(elapsedTime);
    if (_formEngine) _formEngine->update(elapsedTime);
    if (_formBraking) _formBraking->update(elapsedTime);
}

void AudioSample::render(float elapsedTime)
{
    // Clear the color and depth buffers
    clear(CLEAR_COLOR_DEPTH, Vector4::zero(), 1.0f, 0);

    // Draw all the forms
    _formBackground->draw();
    _formEngine->draw();
    _formBraking->draw();
}

void AudioSample::controlEvent(Control* control, EventType eventType)
{
    switch (eventType)
    {
        case Control::Listener::RELEASE:
        {
            std::string controlId = control->getId();
            if (controlId == "playButton")
            {
                _audioBackground->play();
            }
            else if (controlId == "pauseButton")
            {
                _audioBackground->pause();
            }
            else if (controlId == "resumeButton")
            {
                _audioBackground->resume();
            }
            else if (controlId == "rewindButton")
            {
                _audioBackground->rewind();
            }
            else if (controlId == "stopButton")
            {
                _audioBackground->stop();
            }
            else if (controlId == "playEngineButton")
            {
                _audioEngine->play();
            }
            else if (controlId == "stopEngineButton")
            {
                _audioEngine->stop();
            }
            else if (controlId == "playBrakingButton")
            {
                _audioBraking->play();
            }
            else if (controlId == "stopBrakingButton")
            {
                _audioBraking->stop();
            }
        }
        break;
        case Control::Listener::VALUE_CHANGED:
        {
            std::string controlId = control->getId();
            if (controlId == "loopCheckBox")
            {
                CheckBox* loopCheckBox = static_cast<CheckBox*>(control);
                _audioBackground->setLooped(loopCheckBox->isChecked());
            }
            else if (controlId == "gainSlider")
            {
                Slider* gainSlider = static_cast<Slider*>(control);
                _audioBackground->setGain(float(gainSlider->getValue()));
            }
            else if (controlId == "pitchSlider")
            {
                Slider* pitchSlider = static_cast<Slider*>(control);
                float pitchValue = (float)pitchSlider->getValue();
                if (pitchValue != 0.0f)
                {
                    _audioBackground->setPitch(pitchValue);
                }
            }
            else if (controlId == "loopEngineCheckBox")
            {
                CheckBox* loopCheckBox = static_cast<CheckBox*>(control);
                _audioEngine->setLooped(loopCheckBox->isChecked());
            }
            else if (controlId == "gainEngineSlider")
            {
                Slider* gainSlider = static_cast<Slider*>(control);
                _audioEngine->setGain(float(gainSlider->getValue()));
            }
            else if (controlId == "pitchEngineSlider")
            {
                Slider* pitchSlider = static_cast<Slider*>(control);
                float pitchValue = (float)pitchSlider->getValue();
                if (pitchValue != 0.0f)
                {
                    _audioEngine->setPitch(pitchValue);
                }
            }
            else if (controlId == "loopBrakingCheckBox")
            {
                CheckBox* loopCheckBox = static_cast<CheckBox*>(control);
                _audioBraking->setLooped(loopCheckBox->isChecked());
            }
            else if (controlId == "gainBrakingSlider")
            {
                Slider* gainSlider = static_cast<Slider*>(control);
                _audioBraking->setGain(float(gainSlider->getValue()));
            }
            else if (controlId == "pitchBrakingSlider")
            {
                Slider* pitchSlider = static_cast<Slider*>(control);
                float pitchValue = (float)pitchSlider->getValue();
                if (pitchValue != 0.0f)
                {
                    _audioBraking->setPitch(pitchValue);
                }
            }
        }
        break;
    }
}
