/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../../sjf_audio/sjf_multislider.h"
#include "../../sjf_audio/sjf_LookAndFeel.h"

//==============================================================================
/**
*/
class Sjf_spectralProcessorAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    Sjf_spectralProcessorAudioProcessorEditor (Sjf_spectralProcessorAudioProcessor&);
    ~Sjf_spectralProcessorAudioProcessorEditor() override;

    //==============================================================================
    void paint (juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;
    
private:
    // This reference is provided as a quick way for your editor to
    // access the processor object that created it.
    Sjf_spectralProcessorAudioProcessor& audioProcessor;

    sjf_lookAndFeel otherLookAndFeel;
    
    sjf_multislider bandGainsMultiSlider, lfoDepthMultiSlider, lfoRateMultiSlider, lfoOffsetMultiSlider;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sjf_spectralProcessorAudioProcessorEditor)
};
