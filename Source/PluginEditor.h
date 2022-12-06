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


struct ResponseCurveComponent : juce::Component,
	juce::AudioProcessorParameter::Listener,
	juce::Timer
{
	ResponseCurveComponent(Simple_eqAudioProcessor&);
	~ResponseCurveComponent();


	void parameterValueChanged(int parameterIndex, float newValue) override;
	void parameterGestureChanged(int parameterIndex, bool gestureIsStarting) override {}
	void timerCallback() override;

	void paint(juce::Graphics&) override;

private:
	Simple_eqAudioProcessor& audioProcessor;
	juce::Atomic<bool> parametersChanged{ false };
	MonoChain monoChain;
};


//==============================================================================
/**
*/
class Simple_eqAudioProcessorEditor : public juce::AudioProcessorEditor

{
public:
    Simple_eqAudioProcessorEditor (Simple_eqAudioProcessor&);
    ~Simple_eqAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Simple_eqAudioProcessor& audioProcessor;

	CustomRotarySlider  peakFreqSlider, 
		                peakGainSlider, 
		                peakQSlider, 
		                loCutFreqSlider, 
		                hiCutFreqSlider, 
		                loCutSlopeSlider, 
		                hiCutSlopeSlider;

	ResponseCurveComponent responseCurveComponent;

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


   JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Simple_eqAudioProcessorEditor)
};
