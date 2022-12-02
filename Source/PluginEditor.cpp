/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Simple_eqAudioProcessorEditor::Simple_eqAudioProcessorEditor (Simple_eqAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{

	for (auto comp : getComps())
	{
		addAndMakeVisible(comp);
	}

    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize (600, 400);
}

Simple_eqAudioProcessorEditor::~Simple_eqAudioProcessorEditor()
{
}

//==============================================================================
void Simple_eqAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    //g.setColour (juce::Colours::white);
    //g.setFont (15.0f);
    //g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}

void Simple_eqAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

	auto bounds = getLocalBounds();
	auto responseArea = bounds.removeFromTop(  bounds.getHeight() * 0.33 );
    auto loCutArea = bounds.removeFromLeft(  bounds.getWidth() * 0.33 );
	auto hiCutArea = bounds.removeFromRight(bounds.getWidth() * 0.5);

	loCutFreqSlider.setBounds(loCutArea.removeFromTop(loCutArea.getHeight() * 0.5));
	loCutSlopeSlider.setBounds(loCutArea);
	hiCutFreqSlider.setBounds(hiCutArea.removeFromTop(hiCutArea.getHeight() * 0.5));
    hiCutSlopeSlider.setBounds(hiCutArea);

	peakFreqSlider.setBounds(bounds.removeFromTop(bounds.getHeight()*0.33));
	peakGainSlider.setBounds(bounds.removeFromTop(bounds.getHeight()*0.5));
	peakQSlider.setBounds(bounds);
}


std::vector<juce::Component*> Simple_eqAudioProcessorEditor::getComps()
{
	return
	{
		&peakFreqSlider,
		&peakGainSlider,
		&peakQSlider,
		&loCutFreqSlider,
		&hiCutFreqSlider,
		&loCutSlopeSlider, 
		&hiCutSlopeSlider
	};
}
