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

	void updatePeakFilter(const ChainSettings chainSettings);

	using Coefficients = Filter::CoefficientsPtr;
	void updateCoefficients(Coefficients &old, const Coefficients &replacements);


	template<int Index, typename ChainType, typename CoefficientType>
	void update(ChainType& chain, CoefficientType& coefficients)
	{
      updateCoefficients(chain.template get<Index>().coefficients, coefficients[Index]);
	  chain.template setBypassed<Index>(false);
	}


	template<typename ChainType, typename CoefficientType>
	void updateCutFilter(ChainType& leftLoCut, const CoefficientType&  cutCoefficients, const int loCutSlope)    //const ChainSettings& chainSettings)
	{

		leftLoCut.template setBypassed<0>(true);
		leftLoCut.template setBypassed<1>(true);
		leftLoCut.template setBypassed<2>(true);
		leftLoCut.template setBypassed<3>(true);

		switch (loCutSlope)
		{
			case Slope_48:
				update<3>(leftLoCut, cutCoefficients);
			case Slope_36:
				update<2>(leftLoCut, cutCoefficients);
			case Slope_24:
				update<1>(leftLoCut, cutCoefficients);
			case Slope_12:
				update<0>(leftLoCut, cutCoefficients);
		}

	}


    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Simple_eqAudioProcessor)
};
