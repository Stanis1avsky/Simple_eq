/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
Simple_eqAudioProcessor::Simple_eqAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       )
#endif
{
}

Simple_eqAudioProcessor::~Simple_eqAudioProcessor()
{
}

//==============================================================================
const juce::String Simple_eqAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Simple_eqAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Simple_eqAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Simple_eqAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Simple_eqAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Simple_eqAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Simple_eqAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Simple_eqAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Simple_eqAudioProcessor::getProgramName (int index)
{
    return {};
}

void Simple_eqAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}



ChainSettings  getChainSettings(juce::AudioProcessorValueTreeState& apvts)
{
	ChainSettings  settings;

	settings.hiCutFreq = apvts.getRawParameterValue("HiCut Freq")->load();
	settings.loCutFreq = apvts.getRawParameterValue("LoCut Freq")->load();
//	settings.hiCutSlope = static_cast<Slope>(apvts.getRawParameterValue("HiCut Slope")->load());
//	settings.loCutSlope = static_cast<Slope>(apvts.getRawParameterValue("LoCut Slope")->load());
	settings.peakFreq = apvts.getRawParameterValue("Peak Freq")->load();
	settings.peakGain = apvts.getRawParameterValue("Peak Gain")->load();
	settings.peakQ = apvts.getRawParameterValue("Peak Q")->load();
	settings.loCutSlope = apvts.getRawParameterValue("LoCut Slope")->load();
	settings.hiCutSlope = apvts.getRawParameterValue("HiCut Slope")->load();

	return settings;
}

//==============================================================================
void Simple_eqAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..

	juce::dsp::ProcessSpec spec;

	spec.maximumBlockSize = samplesPerBlock;
	spec.numChannels = 1;
	spec.sampleRate = sampleRate;

	leftChain.prepare(spec);
	rightChain.prepare(spec);

	auto chainSettings = getChainSettings(apvts);

	auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, chainSettings.peakFreq, chainSettings.peakQ, juce::Decibels::decibelsToGain(chainSettings.peakGain));

	*leftChain.get<ChainPositions::Peak>().coefficients  = *peakCoefficients;
    *rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

	auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.loCutFreq, sampleRate, (chainSettings.loCutSlope+1)*2);

	auto& leftLoCut  = leftChain.get<ChainPositions::LoCut>();

	leftLoCut.setBypassed<0>(true);
	leftLoCut.setBypassed<1>(true);
	leftLoCut.setBypassed<2>(true);
	leftLoCut.setBypassed<3>(true);

	switch (chainSettings.loCutSlope)
	{
	case Slope_12:
		{
			*leftLoCut.get<0>().coefficients = *cutCoefficients[0];
			leftLoCut.setBypassed<0>(false);
			break;
		}
	case Slope_24:
		{
			*leftLoCut.get<0>().coefficients = *cutCoefficients[0];
			leftLoCut.setBypassed<0>(false);
			*leftLoCut.get<1>().coefficients = *cutCoefficients[1];
			leftLoCut.setBypassed<1>(false);
			break;
		}
	case Slope_36:
		{
			*leftLoCut.get<0>().coefficients = *cutCoefficients[0];
			leftLoCut.setBypassed<0>(false);
			*leftLoCut.get<1>().coefficients = *cutCoefficients[1];
			leftLoCut.setBypassed<1>(false);
			*leftLoCut.get<2>().coefficients = *cutCoefficients[2];
			leftLoCut.setBypassed<2>(false);
			break;
		}
	case Slope_48:
		{
			*leftLoCut.get<0>().coefficients = *cutCoefficients[0];
			leftLoCut.setBypassed<0>(false);
			*leftLoCut.get<1>().coefficients = *cutCoefficients[1];
			leftLoCut.setBypassed<1>(false);
			*leftLoCut.get<2>().coefficients = *cutCoefficients[2];
			leftLoCut.setBypassed<2>(false);
			*leftLoCut.get<3>().coefficients = *cutCoefficients[3];
			leftLoCut.setBypassed<3>(false);
			break;
		}
	}

	auto& rightLoCut = rightChain.get<ChainPositions::LoCut>();

	rightLoCut.setBypassed<0>(true);
	rightLoCut.setBypassed<1>(true);
	rightLoCut.setBypassed<2>(true);
	rightLoCut.setBypassed<3>(true);

	switch (chainSettings.loCutSlope)
	{
	case Slope_12:
		{
			*rightLoCut.get<0>().coefficients = *cutCoefficients[0];
			rightLoCut.setBypassed<0>(false);
			break;
		}
	case Slope_24:
		{
			*rightLoCut.get<0>().coefficients = *cutCoefficients[0];
			rightLoCut.setBypassed<0>(false);
			*rightLoCut.get<1>().coefficients = *cutCoefficients[1];
			rightLoCut.setBypassed<1>(false);
			break;
		}
	case Slope_36:
		{
			*rightLoCut.get<0>().coefficients = *cutCoefficients[0];
			rightLoCut.setBypassed<0>(false);
			*rightLoCut.get<1>().coefficients = *cutCoefficients[1];
			rightLoCut.setBypassed<1>(false);
			*rightLoCut.get<2>().coefficients = *cutCoefficients[2];
			rightLoCut.setBypassed<2>(false);
			break;
		}
	case Slope_48:
		{
			*rightLoCut.get<0>().coefficients = *cutCoefficients[0];
			rightLoCut.setBypassed<0>(false);
			*rightLoCut.get<1>().coefficients = *cutCoefficients[1];
			rightLoCut.setBypassed<1>(false);
			*rightLoCut.get<2>().coefficients = *cutCoefficients[2];
			rightLoCut.setBypassed<2>(false);
			*rightLoCut.get<3>().coefficients = *cutCoefficients[3];
			rightLoCut.setBypassed<3>(false);
			break;
		}
	}

}

void Simple_eqAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Simple_eqAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    // This is the place where you check if the layout is supported.
    // In this template code we only support mono or stereo.
    // Some plugin hosts, such as certain GarageBand versions, will only
    // load plugins that support stereo bus layouts.
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

    // This checks if the input layout matches the output layout
   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void Simple_eqAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    ////////////// This is the place where you'd normally do the guts of your plugin's
    ////////////// audio processing...
    ////////////// Make sure to reset the state if your inner loop is processing
    ////////////// the samples and the outer loop is handling the channels.
    ////////////// Alternatively, you can process the samples with the channels
    ////////////// interleaved by keeping the same state.
    ////////////for (int channel = 0; channel < totalNumInputChannels; ++channel)
    ////////////{
    ////////////    auto* channelData = buffer.getWritePointer (channel);

    ////////////    // ..do something to the data...
    ////////////}

	auto chainSettings = getChainSettings(apvts);

	auto peakCoefficients = juce::dsp::IIR::Coefficients<float>::makePeakFilter(getSampleRate(), chainSettings.peakFreq, chainSettings.peakQ, juce::Decibels::decibelsToGain(chainSettings.peakGain));

	*leftChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;
	*rightChain.get<ChainPositions::Peak>().coefficients = *peakCoefficients;

	auto cutCoefficients = juce::dsp::FilterDesign<float>::designIIRHighpassHighOrderButterworthMethod(chainSettings.loCutFreq, getSampleRate(), (chainSettings.loCutSlope + 1) * 2);

	auto& leftLoCut = leftChain.get<ChainPositions::LoCut>();

	leftLoCut.setBypassed<0>(true);
	leftLoCut.setBypassed<1>(true);
	leftLoCut.setBypassed<2>(true);
	leftLoCut.setBypassed<3>(true);

	switch (chainSettings.loCutSlope)
	{
	case Slope_12:
	{
		*leftLoCut.get<0>().coefficients = *cutCoefficients[0];
		leftLoCut.setBypassed<0>(false);
		break;
	}
	case Slope_24:
	{
		*leftLoCut.get<0>().coefficients = *cutCoefficients[0];
		leftLoCut.setBypassed<0>(false);
		*leftLoCut.get<1>().coefficients = *cutCoefficients[1];
		leftLoCut.setBypassed<1>(false);
		break;
	}
	case Slope_36:
	{
		*leftLoCut.get<0>().coefficients = *cutCoefficients[0];
		leftLoCut.setBypassed<0>(false);
		*leftLoCut.get<1>().coefficients = *cutCoefficients[1];
		leftLoCut.setBypassed<1>(false);
		*leftLoCut.get<2>().coefficients = *cutCoefficients[2];
		leftLoCut.setBypassed<2>(false);
		break;
	}
	case Slope_48:
	{
		*leftLoCut.get<0>().coefficients = *cutCoefficients[0];
		leftLoCut.setBypassed<0>(false);
		*leftLoCut.get<1>().coefficients = *cutCoefficients[1];
		leftLoCut.setBypassed<1>(false);
		*leftLoCut.get<2>().coefficients = *cutCoefficients[2];
		leftLoCut.setBypassed<2>(false);
		*leftLoCut.get<3>().coefficients = *cutCoefficients[3];
		leftLoCut.setBypassed<3>(false);
		break;
	}
	}

	auto& rightLoCut = rightChain.get<ChainPositions::LoCut>();

	rightLoCut.setBypassed<0>(true);
	rightLoCut.setBypassed<1>(true);
	rightLoCut.setBypassed<2>(true);
	rightLoCut.setBypassed<3>(true);

	switch (chainSettings.loCutSlope)
	{
	case Slope_12:
	{
		*rightLoCut.get<0>().coefficients = *cutCoefficients[0];
		rightLoCut.setBypassed<0>(false);
		break;
	}
	case Slope_24:
	{
		*rightLoCut.get<0>().coefficients = *cutCoefficients[0];
		rightLoCut.setBypassed<0>(false);
		*rightLoCut.get<1>().coefficients = *cutCoefficients[1];
		rightLoCut.setBypassed<1>(false);
		break;
	}
	case Slope_36:
	{
		*rightLoCut.get<0>().coefficients = *cutCoefficients[0];
		rightLoCut.setBypassed<0>(false);
		*rightLoCut.get<1>().coefficients = *cutCoefficients[1];
		rightLoCut.setBypassed<1>(false);
		*rightLoCut.get<2>().coefficients = *cutCoefficients[2];
		rightLoCut.setBypassed<2>(false);
		break;
	}
	case Slope_48:
	{
		*rightLoCut.get<0>().coefficients = *cutCoefficients[0];
		rightLoCut.setBypassed<0>(false);
		*rightLoCut.get<1>().coefficients = *cutCoefficients[1];
		rightLoCut.setBypassed<1>(false);
		*rightLoCut.get<2>().coefficients = *cutCoefficients[2];
		rightLoCut.setBypassed<2>(false);
		*rightLoCut.get<3>().coefficients = *cutCoefficients[3];
		rightLoCut.setBypassed<3>(false);
		break;
	}
	}




	juce::dsp::AudioBlock<float> block(buffer);

	auto leftBlock  = block.getSingleChannelBlock(0);
	auto rightBlock = block.getSingleChannelBlock(1);

	juce::dsp::ProcessContextReplacing<float> leftContext (leftBlock);
	juce::dsp::ProcessContextReplacing<float> rightContext(rightBlock);

	leftChain.process(leftContext);
    rightChain.process(rightContext);
}

//==============================================================================
bool Simple_eqAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Simple_eqAudioProcessor::createEditor()
{
    //return new Simple_eqAudioProcessorEditor (*this);

	return new juce::GenericAudioProcessorEditor(*this);
}

//==============================================================================
void Simple_eqAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
}

void Simple_eqAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}





juce::AudioProcessorValueTreeState::ParameterLayout Simple_eqAudioProcessor::createParameterLayout()
{
	juce::AudioProcessorValueTreeState::ParameterLayout layout;

	layout.add(std::make_unique<juce::AudioParameterFloat>("LoCut Freq", 
		                                                   "LoCut Freq", 
		                                                   juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f), 
		                                                   20.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("HiCut Freq",
														   "HiCut Freq",
														   juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
														   20000.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Freq",
														   "Peak Freq",
														   juce::NormalisableRange<float>(20.f, 20000.f, 1.f, 0.25f),
														   750.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Gain",
														   "Peak Gain",
														   juce::NormalisableRange<float>(-24.f, 24.f, 0.5f, 1.f),
														   0.f));
	layout.add(std::make_unique<juce::AudioParameterFloat>("Peak Q",
														   "Peak Q",
														   juce::NormalisableRange<float>(0.1f, 10.f, 0.05f, 1.f),
														   1.f));

	juce::StringArray stringArray;
	for (int i = 0; i < 4; i++)
	{
		juce::String str;
		str << (12 + i * 12)<<" dB/Oct";
		stringArray.add(str);
	}

	layout.add(std::make_unique<juce::AudioParameterChoice>("LoCut Slope", "LoCut Slope", stringArray, 0));
	layout.add(std::make_unique<juce::AudioParameterChoice>("HiCut Slope", "HiCut Slope", stringArray, 0));

	return layout;
}



//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Simple_eqAudioProcessor();
}
