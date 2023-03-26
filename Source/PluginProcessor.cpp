/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <math.h>

//==============================================================================
Sjf_spectralProcessorAudioProcessor::Sjf_spectralProcessorAudioProcessor()
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
, parameters(*this, nullptr, juce::Identifier("sjf_spectralProcessor"), createParameterLayout() )
{
    
//    bandGainParameter.resize( NUM_BANDS );
//    lfoRateParameter.resize( NUM_BANDS );
//    lfoDepthParameter.resize( NUM_BANDS );
//    lfoOffsetParameter.resize( NUM_BANDS );
//
//    m_lfos.resize( NUM_BANDS );
//    for ( int c = 0; c < m_filters.size(); c++ )
//    {
//        m_filters[ c ].resize( NUM_BANDS );
//    }
    
    initialiseFilters( getSampleRate() );
    
    for ( int f = 0; f < NUM_BANDS; f++ )
    {
        bandGainParameter[ f ] = parameters.state.getPropertyAsValue("bandGain" + juce::String( f ), nullptr, true);
        lfoRateParameter[ f ] = parameters.state.getPropertyAsValue("lfoRate" + juce::String( f ), nullptr, true);
        lfoDepthParameter[ f ] = parameters.state.getPropertyAsValue("lfoDepth" + juce::String( f ), nullptr, true);
        lfoOffsetParameter[ f ] = parameters.state.getPropertyAsValue("lfoOffset" + juce::String( f ), nullptr, true);
    }
}

Sjf_spectralProcessorAudioProcessor::~Sjf_spectralProcessorAudioProcessor()
{
}

//==============================================================================
const juce::String Sjf_spectralProcessorAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool Sjf_spectralProcessorAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool Sjf_spectralProcessorAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool Sjf_spectralProcessorAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double Sjf_spectralProcessorAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int Sjf_spectralProcessorAudioProcessor::getNumPrograms()
{
    return 1;   // NB: some hosts don't cope very well if you tell them there are 0 programs,
                // so this should be at least 1, even if you're not really implementing programs.
}

int Sjf_spectralProcessorAudioProcessor::getCurrentProgram()
{
    return 0;
}

void Sjf_spectralProcessorAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String Sjf_spectralProcessorAudioProcessor::getProgramName (int index)
{
    return {};
}

void Sjf_spectralProcessorAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void Sjf_spectralProcessorAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Use this method as the place to do any pre-playback
    // initialisation that you need..
    initialiseFilters( sampleRate );
}

void Sjf_spectralProcessorAudioProcessor::releaseResources()
{
    // When playback stops, you can use this as an opportunity to free up any
    // spare memory, etc.
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool Sjf_spectralProcessorAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
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

void Sjf_spectralProcessorAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    auto bufferSize = buffer.getNumSamples();
    // In case we have more outputs than inputs, this code clears any output
    // channels that didn't contain input data, (because these aren't
    // guaranteed to be empty - they may contain garbage).
    // This is here to avoid people getting screaming feedback
    // when they first compile a plugin, but obviously you don't need to keep
    // this code if your algorithm always overwrites all the output channels.
    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    
    for ( int f = 0; f < NUM_BANDS; f++ )
    {
        auto r = (0.01f * std::pow( 2000.0f, (float)lfoRateParameter[ f ].getValue() ));
//        auto param =  (float)lfoRateParameter[ f ].getValue();
//        DBG("rate " << r << " param " << param );
        m_lfos[ f ].setRateChange( 1.0f/r );
        m_lfos[ f ].setOffset( sjf_scale<float>(0, 1, -0.5, 0.5, lfoOffsetParameter[ f ].getValue() ) );
        m_lfos[ f ].setLFOtype( sjf_lfo::lfoType::sine );
    }
    
    
    
    std::array< double, NUM_BANDS > filteredAudio, lfoOutputs;
    double samp;
    for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
    {
        for ( int f = 0; f < NUM_BANDS; f++ )
        {
            lfoOutputs[ f ] = m_lfos[ f ].output() * (float)lfoDepthParameter[ f ].getValue();
        }
        // do lfo and weirdness here
        for ( int channel = 0; channel < totalNumOutputChannels; channel++ )
        {
            samp = buffer.getSample( fastMod( channel, totalNumInputChannels ), indexThroughBuffer );
            for ( int f = 0; f < NUM_BANDS; f++ )
            {
                filteredAudio[ f ] = m_filters[ channel ][ f ].filterInput( samp );
                filteredAudio[ f ] *= lfoOutputs[ f ] + (float)bandGainParameter[ f ].getValue();
            }
            samp = 0;
            for ( int f = 0; f < NUM_BANDS; f++ )
            {
                samp += filteredAudio[ f ];
            }
            buffer.setSample( channel, indexThroughBuffer, samp );
        }
    }
}

//==============================================================================
bool Sjf_spectralProcessorAudioProcessor::hasEditor() const
{
    return true; // (change this to false if you choose to not supply an editor)
}

juce::AudioProcessorEditor* Sjf_spectralProcessorAudioProcessor::createEditor()
{
    return new Sjf_spectralProcessorAudioProcessorEditor (*this);
}

//==============================================================================
void Sjf_spectralProcessorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
//    for ( int f = 0; f < NUM_BANDS; f++ )
//    {
//        bandGainParameter[ f ].setValue( sampleMangler2.getSampleChoiceProbability( v ) );
//    }
}

void Sjf_spectralProcessorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
}

//==============================================================================

void Sjf_spectralProcessorAudioProcessor::initialiseFilters( double sampleRate)
{
    //    m_biquadCalculator.initialise( sampleRate );
    
    static constexpr std::array< double, NUM_BANDS > frequencies
    { 100 , 150, 250, 350, 500, 630, 800, 1000, 1300, 1600, 2000, 2600, 3500, 5000, 8000, 10000 };
//    { 1000 };
    
    for ( int c = 0; c < m_filters.size(); c++ )
    {
        for ( int f = 0; f < NUM_BANDS; f++ )
        {
            m_filters[ c ][ f ].initialise( sampleRate );
            m_filters[ c ][ f ].setNumOrders( ORDER );
            m_filters[ c ][ f ].setFrequency( frequencies[ f ] );
            if ( f == 0 ){ m_filters[ c ][ f ].setFilterType( sjf_biquadCalculator<double>::filterType::lowpass ); }
            else if ( f == NUM_BANDS-1 ){ m_filters[ c ][ f ].setFilterType( sjf_biquadCalculator<double>::filterType::highpass ); }
            else { m_filters[ c ][ f ].setFilterType( sjf_biquadCalculator<double>::filterType::bandpass ); }
        }
    }
}
//==============================================================================

void Sjf_spectralProcessorAudioProcessor::initialiseLFOs( double sampleRate)
{
    //    m_biquadCalculator.initialise( sampleRate );
    
    for ( int f = 0; f < NUM_BANDS; f++ )
    {
        m_lfos[ f ].setSampleRate( sampleRate );
    }
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setBandGain( const int bandNumber, const double gain )
{
    bandGainParameter[ bandNumber ].setValue( gain );
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getBandGain( const int bandNumber )
{
    return bandGainParameter[ bandNumber ].getValue();
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFORate( const int bandNumber, const double lfoR )
{
    lfoRateParameter[ bandNumber ].setValue( lfoR );
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFORate( const int bandNumber )
{
    return lfoRateParameter[ bandNumber ].getValue();
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFODepth( const int bandNumber, const double lfoD )
{
    lfoDepthParameter[ bandNumber ].setValue( lfoD );
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFODepth( const int bandNumber )
{
    return lfoDepthParameter[ bandNumber ].getValue();
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFOOffset( const int bandNumber, const double lfoOffset )
{
    lfoOffsetParameter[ bandNumber ].setValue( lfoOffset );
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFOOffset( const int bandNumber )
{
    return lfoOffsetParameter[ bandNumber ].getValue();
}
//==============================================================================

juce::AudioProcessorValueTreeState::ParameterLayout Sjf_spectralProcessorAudioProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    
    return params;
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Sjf_spectralProcessorAudioProcessor();
}
