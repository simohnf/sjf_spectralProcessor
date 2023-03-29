/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin processor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <math.h>

#define NUM_CHANNELS 2
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
    lfoTypeParameter = parameters.getRawParameterValue("lfoType");
    bandsParameter = parameters.getRawParameterValue("bands");
    filterDesignParameter = parameters.getRawParameterValue("filterDesign");
    filterOrderParameter = parameters.getRawParameterValue("filterOrder");
    
    initialiseFilters( getSampleRate() );
    initialiseDelayLines( getSampleRate() );
    initialiseLFOs( getSampleRate() );
    initialiseSmoothers( getSampleRate() );
    

    
//    bandGainParameter.resize(NUM_BANDS);
//    lfoRateParameter.resize(NUM_BANDS);
//    lfoDepthParameter.resize(NUM_BANDS);
//    lfoOffsetParameter.resize(NUM_BANDS);
//    delayTimeParameter.resize(NUM_BANDS);
//    feedbackParameter.resize(NUM_BANDS);
    

    
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        bandGainParameter[ b ] = parameters.state.getPropertyAsValue("bandGain" + juce::String( b ), nullptr, true);
        polarityParameter[ b ] = parameters.state.getPropertyAsValue("polarity" + juce::String( b ), nullptr, true);
        lfoRateParameter[ b ] = parameters.state.getPropertyAsValue("lfoRate" + juce::String( b ), nullptr, true);
        lfoDepthParameter[ b ] = parameters.state.getPropertyAsValue("lfoDepth" + juce::String( b ), nullptr, true);
        lfoOffsetParameter[ b ] = parameters.state.getPropertyAsValue("lfoOffset" + juce::String( b ), nullptr, true);
        
        delayTimeParameter[ b ] = parameters.state.getPropertyAsValue("delayTime" + juce::String( b ), nullptr, true);
        feedbackParameter[ b ] = parameters.state.getPropertyAsValue("feedback" + juce::String( b ), nullptr, true);
        
        delaysOnOffParameter[ b ] = parameters.state.getPropertyAsValue("delayOnOff" + juce::String( b ), nullptr, true);
        lfosOnOffParameter[ b ] = parameters.state.getPropertyAsValue("lfoOnOff" + juce::String( b ), nullptr, true);
    }
    
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        bandGainParameter[ b ].setValue( rand01() );
        polarityParameter[ b ].setValue( std::round( rand01() ) );
        lfoRateParameter[ b ].setValue( rand01() );
        lfoDepthParameter[ b ].setValue( rand01() );
        lfoOffsetParameter[ b ].setValue( rand01() );
        delayTimeParameter[ b ].setValue( rand01() );
        feedbackParameter[ b ].setValue( rand01() );
        
        delaysOnOffParameter[ b ].setValue( 0 );
        lfosOnOffParameter[ b ].setValue( 0 );
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
    initialiseDelayLines( sampleRate );
    initialiseLFOs( sampleRate );
    initialiseSmoothers( sampleRate );
    
    float dcCutoff = calculateLPFCoefficient< float > ( 15, sampleRate );
    for ( int c = 0; c < dcFilter.size(); c++ )
    {
        dcFilter[ c ].setCutoff( dcCutoff );
    }
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

    std::array< float, NUM_BANDS > lfoDepths, bandGainTargets, feedbackTarget, delayTimeTarget;
    std::array< bool, NUM_BANDS > polarityFlip, lfoOn, delayOn;
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        auto r = (0.01f * std::pow( 2000.0f, (float)lfoRateParameter[ b ].getValue() ));
        m_lfos[ b ].setRateChange( 1.0f/r );
        m_lfos[ b ].setOffset( sjf_scale<float>(0, 1, -0.5, 0.5, lfoOffsetParameter[ b ].getValue() ) );
        m_lfos[ b ].setLFOtype( sjf_lfo::lfoType::sine );
        int lfotyp = *lfoTypeParameter;
        if ( lfotyp == 2 ){ lfotyp = sjf_lfo::lfoType::noise2; }
        m_lfos[ b ].setLFOtype( lfotyp );
        
        lfoDepths[ b ] = (float)lfoDepthParameter[ b ].getValue();
        bandGainTargets[ b ] = (float)bandGainParameter[ b ].getValue();
        
        // a little bit of scaling just to keep delay reasonable
        for ( int c = 0; c < 2; c++ )
        {
            delayTimeTarget[ b ] = 1.0f + 0.1f * (float)delayTimeParameter[ b ].getValue() * getSampleRate();
        }
        feedbackTarget[ b ] = (float)feedbackParameter[ b ].getValue() * 0.999f;
        
        polarityFlip[ b ] = (bool)polarityParameter[ b ].getValue();
        
        lfoOn[ b ] = (bool)lfosOnOffParameter[ b ].getValue();
        delayOn[ b ] = (bool)delaysOnOffParameter[ b ].getValue();
        
        if ( !delayOn[ b ] )
        {
            for ( int c = 0; c < m_delayLines.size(); c++ )
            {
                m_delayLines[ c ][ b ].clearDelayline();
            }
        }
    }
    setFilterDesign( *filterDesignParameter );
    setFilterOrder( *filterOrderParameter );
//    polarityParameter
    std::array< float, NUM_BANDS > filteredAudio, lfoOutputs, delayed, feedback, bandGains;
    
    float sampIn, sampOut, gain;
    
    int whichBands = *bandsParameter;
    int bandStart = (whichBands == 3) ? 1 : 0;
    int bandIncrement = (whichBands == 1) ? 1 : 2;
    for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
    {
        for ( int b = 0; b < NUM_BANDS; b++ )
        {
            lfoOutputs[ b ] = m_lfoSmoother[ b ].filterInput( m_lfos[ b ].output() * lfoDepths[ b ] );
            for ( int channel = 0; channel < 2; channel++ )
            {
                m_delayLines[ channel ][ b ].setDelayTimeSamps( m_delaySmoother[ b ].filterInput( delayTimeTarget[ b ] ) );
            }
            feedback[ b ] = m_fbSmoother[ b ].filterInput( feedbackTarget[ b ] );
            gain = polarityFlip[ b ] ? bandGainTargets[ b ] * -1.0f : bandGainTargets[ b ];
            bandGains[ b ] = m_gainSmoother[ b ].filterInput( gain );
        }
        // do lfo and weirdness here
        for ( int channel = 0; channel < totalNumOutputChannels; channel++ )
        {
            sampIn = buffer.getSample( fastMod( channel, totalNumInputChannels ), indexThroughBuffer );
            sampOut = 0;
            for ( int b = 0; b < NUM_BANDS; b++ )
            {
                gain = bandGains[ b ];
                if ( lfoOn[ b ] ) { gain += gain * lfoOutputs[ b ]; }
                filteredAudio[ b ] = m_filters[ channel ][ b ].filterInput( sampIn );
                filteredAudio[ b ] *=  gain;
                if ( delayOn[ b ] )
                {
                    delayed[ b ] = m_delayLines[ channel ][ b ].getSample2();
                    m_delayLines[ channel ][ b ].setSample2( filteredAudio[ b ] + delayed[ b ] * feedback[ b ]);
                }
            }
            // only output odd/even/all bands
            for ( int b = bandStart; b < NUM_BANDS; b += bandIncrement )
            {
                if ( delayOn[ b ] ) { sampOut += delayed[ b ]; }
                else { sampOut += filteredAudio[ b ]; }
            }
            sampOut -= dcFilter[ channel ].filterInputSecondOrder( sampOut );
            buffer.setSample( channel, indexThroughBuffer, sampOut );
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
    return new Sjf_spectralProcessorAudioProcessorEditor ( *this, parameters );
}

//==============================================================================
void Sjf_spectralProcessorAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    // You should use this method to store your parameters in the memory block.
    // You could do that either as raw data, or use the XML or ValueTree classes
    // as intermediaries to make it easy to save and load complex data.
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        bandGainParameter[ b ] = parameters.state.getPropertyAsValue("bandGain" + juce::String( b ), nullptr, true);
        polarityParameter[ b ] = parameters.state.getPropertyAsValue("polarity" + juce::String( b ), nullptr, true);
        lfoRateParameter[ b ] = parameters.state.getPropertyAsValue("lfoRate" + juce::String( b ), nullptr, true);
        lfoDepthParameter[ b ] = parameters.state.getPropertyAsValue("lfoDepth" + juce::String( b ), nullptr, true);
        lfoOffsetParameter[ b ] = parameters.state.getPropertyAsValue("lfoOffset" + juce::String( b ), nullptr, true);
        for ( int c = 0; c < 2; c ++)
        {
            delayTimeParameter[ b ] = parameters.state.getPropertyAsValue("delayTime" + juce::String( b ), nullptr, true);
            feedbackParameter[ b ] = parameters.state.getPropertyAsValue("feedback" + juce::String( b ), nullptr, true);
        }
        
        delaysOnOffParameter[ b ] = parameters.state.getPropertyAsValue("delayOnOff" + juce::String( b ), nullptr, true);
        lfosOnOffParameter[ b ] = parameters.state.getPropertyAsValue("lfoOnOff" + juce::String( b ), nullptr, true);
    }
    auto state = parameters.copyState();
    
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void Sjf_spectralProcessorAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    // You should use this method to restore your parameters from this memory block,
    // whose contents will have been created by the getStateInformation() call.
    
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
    {
        if (xmlState->hasTagName (parameters.state.getType()))
        {
            parameters.replaceState (juce::ValueTree::fromXml (*xmlState));
            for ( int b = 0; b < NUM_BANDS; b++ )
            {
                bandGainParameter[ b ].referTo( parameters.state.getPropertyAsValue( "bandGain"+juce::String( b ), nullptr ) );
                polarityParameter[ b ].referTo( parameters.state.getPropertyAsValue( "polarity"+juce::String( b ), nullptr ) );
                lfoRateParameter[ b ].referTo( parameters.state.getPropertyAsValue( "lfoRate"+juce::String( b ), nullptr ) );
                lfoDepthParameter[ b ].referTo( parameters.state.getPropertyAsValue( "lfoDepth"+juce::String( b ), nullptr ) );
                lfoOffsetParameter[ b ].referTo( parameters.state.getPropertyAsValue( "lfoOffset"+juce::String( b ), nullptr ) );
                for ( int c = 0; c < 2; c ++)
                {
                    delayTimeParameter[ b ].referTo( parameters.state.getPropertyAsValue("delayTime" + juce::String( b ), nullptr) );
                    feedbackParameter[ b ].referTo( parameters.state.getPropertyAsValue("feedback" + juce::String( b ), nullptr) );
                }
                
                delaysOnOffParameter[ b ].referTo( parameters.state.getPropertyAsValue("delayOnOff" + juce::String( b ), nullptr) );
                lfosOnOffParameter[ b ].referTo( parameters.state.getPropertyAsValue("lfoOnOff" + juce::String( b ), nullptr) );
            }
        }
    }
}

//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setFilterDesign( const int filterDesign )
{
    for ( int c = 0; c < m_filters.size(); c++ )
    {
        for ( int f = 0; f < NUM_BANDS; f++ )
        {
            m_filters[ c ][ f ].setFilterDesign( filterDesign );
        }
    }
}
//==============================================================================

void Sjf_spectralProcessorAudioProcessor::setFilterOrder( const int filterOrder )
{
    for ( int c = 0; c < m_filters.size(); c++ )
    {
        for ( int f = 0; f < NUM_BANDS; f++ )
        {
            m_filters[ c ][ f ].setNumOrders( filterOrder );
        }
    }
}
//==============================================================================

void Sjf_spectralProcessorAudioProcessor::initialiseFilters( double sampleRate)
{
    //    m_biquadCalculator.initialise( sampleRate );
    
    static constexpr std::array< double, NUM_BANDS > frequencies
    { 100 , 150, 250, 350, 500, 630, 800, 1000, 1300, 1600, 2000, 2600, 3500, 5000, 8000, 10000 };
//    { 1000 };
    
    setFilterDesign( *filterDesignParameter );
    setFilterOrder( *filterOrderParameter );
    for ( int c = 0; c < m_filters.size(); c++ )
    {
        for ( int f = 0; f < NUM_BANDS; f++ )
        {
            m_filters[ c ][ f ].initialise( sampleRate );
//            m_filters[ c ][ f ].setNumOrders( ORDER );
            
//            m_filters[ c ][ f ].setFilterDesign( sjf_biquadCascade<double>::filterDesign::butterworth );
            if ( f == 0 ){ m_filters[ c ][ f ].setFilterType( sjf_biquadCalculator<double>::filterType::lowpass ); }
            else if ( f == NUM_BANDS-1 ){ m_filters[ c ][ f ].setFilterType( sjf_biquadCalculator<double>::filterType::highpass ); }
            else { m_filters[ c ][ f ].setFilterType( sjf_biquadCalculator<double>::filterType::bandpass ); }
            m_filters[ c ][ f ].setFrequency( frequencies[ f ] );
        }
    }
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::initialiseDelayLines( double sampleRate )
{
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        for ( int c = 0; c < 2; c ++ )
        {
            m_delayLines[ c ][ b ].initialise( sampleRate );
        }
    }
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::initialiseSmoothers( double sampleRate )
{
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        m_delaySmoother[ b ].setCutoff( calculateLPFCoefficient<float>( 0.1f, (float)sampleRate ) );
        m_fbSmoother[ b ].setCutoff( calculateLPFCoefficient<float>( 0.1f, (float)sampleRate ) );
        m_gainSmoother[ b ].setCutoff( calculateLPFCoefficient<float>( 5.0f, (float)sampleRate ) );
        m_lfoSmoother[ b ].setCutoff( calculateLPFCoefficient<float>( 1.0f, (float)sampleRate ) );
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
void Sjf_spectralProcessorAudioProcessor::setBandPolarity( const int bandNumber, const bool flip )
{
    polarityParameter[ bandNumber ].setValue( flip );
}
//==============================================================================
const bool Sjf_spectralProcessorAudioProcessor::getBandPolarity( const int bandNumber )
{
    return polarityParameter[ bandNumber ].getValue();
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
void Sjf_spectralProcessorAudioProcessor::setDelayTime( const int bandNumber, const double delay )
{
    delayTimeParameter[ bandNumber ].setValue( delay );
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getDelayTime( const int bandNumber )
{
    return delayTimeParameter[ bandNumber ].getValue();
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setFeedback( const int bandNumber, const double fb )
{
    feedbackParameter[ bandNumber ].setValue( fb );
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getFeedback( const int bandNumber )
{
    return feedbackParameter[ bandNumber ].getValue();
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setDelayOn( const int bandNumber, const bool delayIsOn )
{
    delaysOnOffParameter[ bandNumber ].setValue( delayIsOn );
}
//==============================================================================
const bool Sjf_spectralProcessorAudioProcessor::getDelayOn( const int bandNumber )
{
    return delaysOnOffParameter[ bandNumber ].getValue( );
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLfoOn( const int bandNumber, const bool lfoIsOn )
{
    lfosOnOffParameter[ bandNumber ].setValue( lfoIsOn );
}
//==============================================================================
const bool Sjf_spectralProcessorAudioProcessor::getLfoOn( const int bandNumber )
{
    return lfosOnOffParameter[ bandNumber ].getValue();
}
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout Sjf_spectralProcessorAudioProcessor::createParameterLayout()
{
    static constexpr int pIDVersionNumber = 1;
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    
//    filterOrder
    params.add( std::make_unique<juce::AudioParameterInt>( juce::ParameterID{ "lfoType", pIDVersionNumber }, "LfoType", 1, 2, 2 ) );
    params.add( std::make_unique<juce::AudioParameterInt>( juce::ParameterID{ "bands", pIDVersionNumber }, "Bands", 1, 3, 1 ) );
    params.add( std::make_unique<juce::AudioParameterInt>( juce::ParameterID{ "filterDesign", pIDVersionNumber }, "FilterDesign", 1, 3, 1 ) );
    params.add( std::make_unique<juce::AudioParameterInt>( juce::ParameterID{ "filterOrder", pIDVersionNumber }, "FilterOrder", 2, 8, 4 ) );
    
    return params;
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Sjf_spectralProcessorAudioProcessor();
}
