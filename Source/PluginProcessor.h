/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "../../sjf_audio/sjf_audioUtilities.h"
#include "../../sjf_audio/sjf_biquadCascade.h"
#include "../../sjf_audio/sjf_lfo.h"

//==============================================================================
/**
*/
class Sjf_spectralProcessorAudioProcessor  : public juce::AudioProcessor
                            #if JucePlugin_Enable_ARA
                             , public juce::AudioProcessorARAExtension
                            #endif
{
public:
    //==============================================================================
    Sjf_spectralProcessorAudioProcessor();
    ~Sjf_spectralProcessorAudioProcessor() override;

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

    void setBandGain( const int bandNumber, const double gain );
    const double getBandGain( const int bandNumber );
private:
    void initialiseFilters( double sampleRate );
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    
    std::array< std::vector < sjf_biquadCascade< double > >, 2 > m_filters;
    std::vector< sjf_lfo > m_lfos;
    
    std::vector< juce::Value > bandGainParameter;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sjf_spectralProcessorAudioProcessor)
};
