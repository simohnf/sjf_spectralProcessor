/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "PluginProcessor.h"
#include "../../sjf_audio/sjf_multislider.h"
#include "../../sjf_audio/sjf_multitoggle.h"
#include "../../sjf_audio/sjf_LookAndFeel.h"
#include "../../sjf_audio/sjf_numBox.h"
//==============================================================================
/**
*/
class Sjf_spectralProcessorAudioProcessorEditor  : public juce::AudioProcessorEditor, public juce::Timer
{
public:
    Sjf_spectralProcessorAudioProcessorEditor (Sjf_spectralProcessorAudioProcessor&, juce::AudioProcessorValueTreeState& vts);
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
    juce::AudioProcessorValueTreeState& valueTreeState;
    
    sjf_lookAndFeel otherLookAndFeel;
    
    juce::ComboBox lfoTypeBox, bandsChoiceBox, filterDesignBox;
    juce::TextButton randomAllButton;
    juce::ToggleButton tooltipsToggle;
    
    juce::Label tooltipLabel;
    
    sjf_multislider bandGainsMultiSlider, lfoDepthMultiSlider, lfoRateMultiSlider, lfoOffsetMultiSlider, delayTimeMultiSlider, feedbackMultiSlider;
    sjf_multitoggle polarityFlips, delaysOnOff, lfosOnOff;
    sjf_numBox filterOrderNumBox;
    
    std::unique_ptr< juce::AudioProcessorValueTreeState::ComboBoxAttachment > lfoTypeBoxAttachment, bandsChoiceBoxAttachment, filterDesignBoxAttachment;
    std::unique_ptr< juce::AudioProcessorValueTreeState::SliderAttachment > filterOrderNumBoxAttachment;
    juce::String MAIN_TOOLTIP = "sjf_spectralProcessor: \n16 band graphic EQ with LFO modulation for gain and feedback delay lines for each band... \nNot designed for functional equalisation, but for sound design\n";
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sjf_spectralProcessorAudioProcessorEditor)
};
