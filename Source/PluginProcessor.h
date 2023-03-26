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

//#define NUM_BANDS 16
#define ORDER 8
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

    
    int getNumBands() { return NUM_BANDS; }
    void setBandGain( const int bandNumber, const double gain );
    const double getBandGain( const int bandNumber );
    
    void setLFORate( const int bandNumber, const double lfoR );
    const double getLFORate( const int bandNumber );
    
    void setLFODepth( const int bandNumber, const double lfoD );
    const double getLFODepth( const int bandNumber );
    
    void setLFOOffset( const int bandNumber, const double lfoOffset );
    const double getLFOOffset( const int bandNumber );
private:
    void initialiseFilters( double sampleRate );
    void initialiseLFOs( double sampleRate );
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    
    static const int NUM_BANDS  = 16;
    
    std::array< std::array < sjf_biquadCascade< double >, NUM_BANDS >, 2 > m_filters;
    std::array< sjf_lfo, NUM_BANDS > m_lfos;
    
    std::array< juce::Value, NUM_BANDS > bandGainParameter, lfoRateParameter, lfoDepthParameter, lfoOffsetParameter;
    
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sjf_spectralProcessorAudioProcessor)
};