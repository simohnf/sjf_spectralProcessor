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
#include "../../sjf_audio/sjf_delayLine.h"
#include "../../sjf_audio/sjf_lpf.h"
#include "../../sjf_audio/sjf_audioUtilities.h"

//#define NUM_BANDS 16
#define ORDER 4
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
    
    void setBandPolarity( const int bandNumber, const bool flip );
    const bool getBandPolarity( const int bandNumber );
    
    void setLFORate( const int bandNumber, const double lfoR );
    const double getLFORate( const int bandNumber );
    
    void setLFODepth( const int bandNumber, const double lfoD );
    const double getLFODepth( const int bandNumber );
    
    void setLFOOffset( const int bandNumber, const double lfoOffset );
    const double getLFOOffset( const int bandNumber );
    
    void setDelayTime( const int bandNumber, const double delay );
    const double getDelayTime( const int bandNumber );
    
    void setFeedback( const int bandNumber, const double fb );
    const double getFeedback( const int bandNumber );
    
    void setDelayMix( const int bandNumber, const double delayMix );
    const double getDelayMix( const int bandNumber );
    
    void setDelayOn( const int bandNumber, const bool delayIsOn );
    const bool getDelayOn( const int bandNumber );
    
    void setLfoOn( const int bandNumber, const bool lfoIsOn );
    const bool getLfoOn( const int bandNumber );
    
    void setParametersChangedFalse() { m_parametersChangedFlag = false; }
    bool checkIfParametersChanged() { return m_parametersChangedFlag; }
    
    
    void setBandGain( const int presetNumber, const int bandNumber, const double gain );
    const double getBandGain( const int presetNumber, const int bandNumber );
    
    void setBandPolarity( const int presetNumber, const int bandNumber, const bool flip );
    const bool getBandPolarity( const int presetNumber, const int bandNumber );
    
    void setLFORate( const int presetNumber, const int bandNumber, const double lfoR );
    const double getLFORate( const int presetNumber, const int bandNumber );
    
    void setLFODepth( const int presetNumber, const int bandNumber, const double lfoD );
    const double getLFODepth( const int presetNumber, const int bandNumber );
    
    void setLFOOffset( const int presetNumber, const int bandNumber, const double lfoOffset );
    const double getLFOOffset( const int presetNumber, const int bandNumber );
    
    void setDelayTime( const int presetNumber, const int bandNumber, const double delay );
    const double getDelayTime( const int presetNumber, const int bandNumber );
    
    void setFeedback( const int presetNumber, const int bandNumber, const double fb );
    const double getFeedback( const int presetNumber, const int bandNumber );
    
    void setDelayMix( const int presetNumber, const int bandNumber, const double delayMix );
    const double getDelayMix( const int presetNumber, const int bandNumber );
    
    void getPreset(const int presetNumber);
    
    void interpolatePresets( std::array< float, 4 > weights );
    
    void isEditorOpen( const bool editorIsOpen ){ m_editorOpenFlag = editorIsOpen; }
private:
    void setFilterDesign( const int filterDesign );
    void setFilterOrder( const int filterOrder );
    void initialiseFilters( double sampleRate );
    void initialiseDelayLines( double sampleRate );
    void initialiseLFOs( double sampleRate );
    void initialiseDCBlock( double sampleRate );
    void initialiseSmoothers( double sampleRate );
    
    
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();
private:
    //==============================================================================
    juce::AudioProcessorValueTreeState parameters;
    
    static const int NUM_BANDS  = 16;
    bool m_parametersChangedFlag = false, m_editorOpenFlag = false;
    
    std::array< std::array < sjf_biquadCascade< float >, NUM_BANDS >, 2 > m_filters;
    std::array< sjf_lfo, NUM_BANDS > m_lfos, m_delayLfos;
    std::array< std::array< sjf_delayLine< float >, NUM_BANDS >, 2 > m_delayLines;
    
    
    std::array< sjf_lpf< float >, NUM_BANDS > m_gainSmoother, m_delaySmoother, m_fbSmoother, m_delayWetSmoother, m_delayDrySmoother, m_lfoSmoother;
    std::array< sjf_lpf< float >, 2 > dcFilter;
    
    std::array< bool, NUM_BANDS > m_polarites, m_delaysOnOff, m_lfosOnOff;
    std::array< float, NUM_BANDS > m_bandGains, m_lfoRates, m_lfoDepths, m_lfoOffsets, m_delayTimes, m_feedbacks, m_delayMix;
    
    std::array< std::array< float, NUM_BANDS >, 4 > m_bandGainsPresets, m_lfoRatesPresets, m_lfoDepthsPresets, m_lfoOffsetsPresets, m_delayTimesPresets, m_feedbacksPresets, m_delayMixPresets;
    
    std::array< std::array< bool, NUM_BANDS >, 4 > m_polarityPresets;
    
    std::array< juce::Value, NUM_BANDS > bandGainParameter, polarityParameter, lfoRateParameter, lfoDepthParameter, lfoOffsetParameter, delayTimeParameter, feedbackParameter, delayMixParameter, delaysOnOffParameter, lfosOnOffParameter;
    
    std::array< std::array< juce::Value, NUM_BANDS >, 4 > bandGainPresetsParameter, polarityPresetsParameter, lfoRatePresetsParameter, lfoDepthPresetsParameter, lfoOffsetPresetsParameter, delayTimePresetsParameter, feedbackPresetsParameter, delayMixPresetsParameter, delaysOnOffPresetsParameter, lfosOnOffPresetsParameter;
    
    std::atomic<float>* lfoTypeParameter = nullptr;
    std::atomic<float>* bandsParameter = nullptr;
    std::atomic<float>* filterDesignParameter = nullptr;
    std::atomic<float>* filterOrderParameter = nullptr;
    std::atomic<float>* xParameter = nullptr;
    std::atomic<float>* yParameter = nullptr;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (Sjf_spectralProcessorAudioProcessor)
};
