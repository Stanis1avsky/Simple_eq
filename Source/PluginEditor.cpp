/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"



void LookAndFeel::drawRotarySlider(juce::Graphics& g,
	int x, int y, int width, int height,
	float sliderPosProportional,
	float rotaryStartAngle,
	float rotaryEndAngle,
	juce::Slider& slider)
{
  using namespace juce;

  auto bounds = Rectangle<float>(x ,y ,width, height);

  g.setColour(Colour(97u, 18u, 167u));
  g.fillEllipse(bounds);

  g.setColour(Colour(255u, 154u, 1u));
  g.drawEllipse(bounds, 1.f);


  if (auto* rswl = dynamic_cast<RotarySliderWithLabels*>(&slider))
  {
	auto center = bounds.getCentre();

	Path p;
	Rectangle<float> r;

	r.setLeft(center.getX() - 2);
	r.setRight(center.getX() + 2);
	r.setTop(bounds.getY());
	r.setBottom(center.getY() - rswl->getTextHeight()*1.5);

	p.addRoundedRectangle(r, 2.f);
	//  p.addRectangle(r);

	jassert(rotaryStartAngle < rotaryEndAngle);

	auto sliderAngRad = jmap(sliderPosProportional, 0.f, 1.f, rotaryStartAngle, rotaryEndAngle);

	p.applyTransform(AffineTransform().rotated(sliderAngRad, center.getX(), center.getY()));

	g.fillPath(p);

	g.setFont(rswl->getTextHeight());
	auto text = rswl->getDisplayString();
	auto strWidth = g.getCurrentFont().getStringWidth(text);

	r.setSize(strWidth + 4, rswl->getTextHeight() + 2);
	r.setCentre(bounds.getCentre());

	g.setColour(juce::Colours::black);
	g.fillRect(r);

	g.setColour(juce::Colours::white);
	g.drawFittedText(text, r.toNearestInt(), juce::Justification::centred, 1);
  }

}

void RotarySliderWithLabels::paint(juce::Graphics& g)
{
	using namespace juce;

	auto startAng = degreesToRadians(180.f + 30.f);
	auto endAng   = degreesToRadians(180.f - 30.f) + MathConstants<float>::twoPi;

	auto range = getRange();
	auto sliderBounds = getSliderBounds();

	//g.setColour(juce::Colours::red);
	//g.drawRect(getLocalBounds());
	//g.setColour(juce::Colours::yellow);
	//g.drawRect(sliderBounds);

	getLookAndFeel().drawRotarySlider(g, sliderBounds.getX(), 
										sliderBounds.getY(), 
										sliderBounds.getWidth(), 
										sliderBounds.getHeight(),
										jmap(getValue(), range.getStart(), range.getEnd(), 0.0, 1.0), 
										startAng, 
										endAng, 
										*this);


}


juce::Rectangle<int> RotarySliderWithLabels::getSliderBounds() const
{
	auto bounds =  getLocalBounds();
	auto size = juce::jmin(bounds.getWidth(), bounds.getHeight());
	size -= getTextHeight() * 2;
	juce::Rectangle<int> r;
	r.setSize(size, size);
	r.setCentre(bounds.getCentreX(), 0);
	r.setY(2);
	return r;
} 



juce::String  RotarySliderWithLabels::getDisplayString() const
{ 
	//return juce::String(getValue());    // Slider::getValue()

	if (auto* choiceParam = dynamic_cast<juce::AudioParameterChoice*>(param))
		return choiceParam->getCurrentChoiceName();

	juce::String str;
	bool addK = false;

	if (auto* floatParam = dynamic_cast<juce::AudioParameterFloat*>(param))
	{
		float value = getValue();
		if (value > 999.f)
		{
			value /= 1000.f;
			addK = true; 
		}

		str = juce::String(value, addK?2:0);

	}
	else
	{
		jassertfalse;
	}

	if (suffix.isNotEmpty())
	{
		str << " ";
		if (addK) str << "k";
		str << suffix;
	}

	return str;
}



ResponseCurveComponent::ResponseCurveComponent(Simple_eqAudioProcessor& p) : audioProcessor(p)
{
	const auto& params = audioProcessor.getParameters();
	for (auto param : params)
	{
		param->addListener(this);
	}

	startTimerHz(60);
}


ResponseCurveComponent::~ResponseCurveComponent()
{
	const auto& params = audioProcessor.getParameters();
	for (auto param : params)
	{
		param->removeListener(this);
	}
}



void ResponseCurveComponent::parameterValueChanged(int parameterIndex, float newValue)
{
	parametersChanged.set(true);
}


void ResponseCurveComponent::timerCallback()
{
	if (parametersChanged.compareAndSetBool(false, true))
	{
		//DBG("params changed");
	  // update the monochain
		auto chainSettings = getChainSettings(audioProcessor.apvts);
		auto peakCoefficients = makePeakFilter(chainSettings, audioProcessor.getSampleRate());
		updateCoefficients(monoChain.get<ChainPositions::Peak>().coefficients, peakCoefficients);

		auto loCutCoefficients = makeLoCutFilter(chainSettings, audioProcessor.getSampleRate());
		auto hiCutCoefficients = makeHiCutFilter(chainSettings, audioProcessor.getSampleRate());
		updateCutFilter(monoChain.get<ChainPositions::LoCut>(), loCutCoefficients, chainSettings.loCutSlope);
		updateCutFilter(monoChain.get<ChainPositions::HiCut>(), hiCutCoefficients, chainSettings.hiCutSlope);

		// signal a repaint
		repaint();
	}
}



void ResponseCurveComponent::paint(juce::Graphics& g)
{
	using namespace juce;

	// (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(Colours::black);     // (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

	auto responseArea = getLocalBounds();
	auto w = responseArea.getWidth();

	auto& locut = monoChain.get<ChainPositions::LoCut>();
	auto& peak = monoChain.get<ChainPositions::Peak>();
	auto& hicut = monoChain.get<ChainPositions::HiCut>();

	auto sampleRate = audioProcessor.getSampleRate();

	std::vector<double>  mags;
	mags.resize(w);

	for (int i = 0; i < w; i++)
	{
		double mag = 1.f;
		auto freq = mapToLog10(double(i) / double(w), 20., 20000.);

		if (!monoChain.isBypassed<ChainPositions::Peak>())
			mag *= peak.coefficients->getMagnitudeForFrequency(freq, sampleRate);

		if (!locut.isBypassed<0>())
			mag *= locut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!locut.isBypassed<1>())
			mag *= locut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!locut.isBypassed<2>())
			mag *= locut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!locut.isBypassed<3>())
			mag *= locut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

		if (!hicut.isBypassed<0>())
			mag *= hicut.get<0>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!hicut.isBypassed<1>())
			mag *= hicut.get<1>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!hicut.isBypassed<2>())
			mag *= hicut.get<2>().coefficients->getMagnitudeForFrequency(freq, sampleRate);
		if (!hicut.isBypassed<3>())
			mag *= hicut.get<3>().coefficients->getMagnitudeForFrequency(freq, sampleRate);

		mags[i] = Decibels::gainToDecibels(mag);
	}

	Path responseCurve;

	const double outputMin = responseArea.getBottom();
	const double outputMax = responseArea.getY();

	auto map = [outputMin, outputMax](double input)
	{
		return jmap(input, -24.0, 24.0, outputMin, outputMax);
	};

	responseCurve.startNewSubPath(responseArea.getX(), map(mags.front()));

	for (size_t i = 0; i < mags.size(); i++)
	{
		responseCurve.lineTo(responseArea.getX() + i, map(mags[i]));
	}

	g.setColour(Colours::beige);
	g.drawRoundedRectangle(responseArea.toFloat(), 4.f, 1.f);

	g.setColour(Colours::white);

	g.strokePath(responseCurve, PathStrokeType(2.f));

	//g.setColour (juce::Colours::white);
	//g.setFont (15.0f);
	//g.drawFittedText ("Hello World!", getLocalBounds(), juce::Justification::centred, 1);
}







//==============================================================================
Simple_eqAudioProcessorEditor::Simple_eqAudioProcessorEditor (Simple_eqAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p),
	peakFreqSlider(*audioProcessor.apvts.getParameter("Peak Freq"), "Hz"),
    peakGainSlider(*audioProcessor.apvts.getParameter("Peak Gain"), "dB"),
	peakQSlider(*audioProcessor.apvts.getParameter("Peak Gain"), ""),
	loCutFreqSlider(*audioProcessor.apvts.getParameter("LoCut Freq"), "Hz"),
	loCutSlopeSlider(*audioProcessor.apvts.getParameter("LoCut Slope"), "dB/Oct"),
	hiCutFreqSlider(*audioProcessor.apvts.getParameter("HiCut Freq"), "Hz"),
	hiCutSlopeSlider(*audioProcessor.apvts.getParameter("HiCut Slope"), "dB/Oct"),
	responseCurveComponent(audioProcessor),
	peakFreqSliderAttachment(audioProcessor.apvts, "Peak Freq", peakFreqSlider),
	peakGainSliderAttachment(audioProcessor.apvts, "Peak Gain", peakGainSlider),
	peakQSliderAttachment(audioProcessor.apvts, "Peak Q", peakQSlider),
	loCutFreqSliderAttachment(audioProcessor.apvts, "LoCut Freq", loCutFreqSlider),
	hiCutFreqSliderAttachment(audioProcessor.apvts, "HiCut Freq", hiCutFreqSlider),
	loCutSlopeSliderAttachment(audioProcessor.apvts, "LoCut Slope", loCutSlopeSlider),
	hiCutSlopeSliderAttachment(audioProcessor.apvts, "HiCut Slope", hiCutSlopeSlider)
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
	using namespace juce;

    // (Our component is opaque, so we must completely fill the background with a solid colour)
	g.fillAll(Colours::black);     // (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

}

void Simple_eqAudioProcessorEditor::resized()
{
    // This is generally where you'll want to lay out the positions of any
    // subcomponents in your editor..

	auto bounds = getLocalBounds();
	auto responseArea = bounds.removeFromTop(  bounds.getHeight() * 0.33 );
	responseCurveComponent.setBounds(responseArea);

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
		&hiCutSlopeSlider,
		&responseCurveComponent
	};
}
