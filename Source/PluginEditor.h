/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"


struct CustomRotarySlider : juce::Slider
{
	CustomRotarySlider() : juce::Slider(juce::Slider::SliderStyle::RotaryHorizontalVerticalDrag, juce::Slider::TextEntryBoxPosition::NoTextBox)
	{

   }
};


//==============================================================================
/**
*/
class Simple_eqAudioProcessorEditor : public juce::AudioProcessorEditor,
	juce::AudioProcessorParameter::Listener,
	juce::Timer
{
public:
    Simple_eqAudioProcessorEditor (Simple_eqAudioProcessor&);
    ~Simple_eqAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
	void timerCallback() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Simple_eqAudioProcessor& audioProcessor;

	juce::Atomic<bool> parametersChanged {false};

	CustomRotarySlider  peakFreqSlider, 
		                peakGainSlider, 
		                peakQSlider, 
		                loCutFreqSlider, 
		                hiCutFreqSlider, 
		                loCutSlopeSlider, 
		                hiCutSlopeSlider;

	using APVTS = juce::AudioProcessorValueTreeState;
	using Attachment = APVTS::SliderAttachment;

	Attachment  peakFreqSliderAttachment,
				peakGainSliderAttachment,
				peakQSliderAttachment,
	    		loCutFreqSliderAttachment,
				hiCutFreqSliderAttachment,
				loCutSlopeSliderAttachment,
				hiCutSlopeSliderAttachment; 

	std::vector<juce::Component*> getComps();

	MonoChain monoChain;


   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Simple_eqAudioProcessorEditor)
};
