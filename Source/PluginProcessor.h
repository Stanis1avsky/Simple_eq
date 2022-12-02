/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


enum Slope
{
	Slope_12,
	Slope_24,
	Slope_36,
	Slope_48
};


struct ChainSettings
{
	float peakFreq{ 0 }, peakGain{ 0 }, peakQ{ 1.f };
	float loCutFreq{ 0 }, hiCutFreq{ 0 };
	int loCutSlope{ Slope::Slope_12 }, hiCutSlope{ Slope::Slope_12 };
};


ChainSettings  getChainSettings(juce::AudioProcessorValueTreeState& apvts);

//==============================================================================
/**
*/
class Simple_eqAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    Simple_eqAudioProcessor();
    ~Simple_eqAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

	static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
	juce::AudioProcessorValueTreeState apvts { *this, nullptr, "Parameters", createParameterLayout()};  // AudioProcessor &processorToConnectTo, UndoManager *undoManagerToUse, const Identifier &ValueTreeType,  ParameterLayout parameterLayout }; 

private:

	using Filter = juce::dsp::IIR::Filter<float>;

	using CutFilter = juce::dsp::ProcessorChain<Filter, Filter, Filter, Filter>;

	using MonoChain = juce::dsp::ProcessorChain<CutFilter, Filter, CutFilter>;

	MonoChain leftChain, rightChain;

	enum ChainPositions
	{
		LoCut,
		Peak,
		HiCut
	};


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Simple_eqAudioProcessor)
};
