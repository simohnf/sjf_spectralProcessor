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
    xParameter = parameters.getRawParameterValue("xyPad-X");
    yParameter = parameters.getRawParameterValue("xyPad-Y");
    
    auto SR = getSampleRate();
    initialiseFilters( SR );
    initialiseDelayLines( SR );
    initialiseLFOs( SR );
    initialiseSmoothers( SR );
    initialiseDCBlock( SR );

    
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        for (int c = 0; c < 2; c++ ) { m_delayLines[ c ][ b ].clearDelayline(); }
        
        m_bandGains[ b ] = 1.0f;
        m_lfoRates[ b ] = 0.5f;
        m_lfoDepths[ b ] = 0.5f;
        m_lfoOffsets[ b ] = 0.5f;
        m_delayTimes[ b ] = 0.5f;
        m_feedbacks[ b ] = 0.5f;
        m_delayMix[ b ] = 0.5f;
        
        for ( int i = 0; i < m_bandGainsPresets.size(); i++ )
        {
            m_bandGainsPresets[ i ][ b ] = 1.0f;
            m_lfoRatesPresets[ i ][ b ] = 0.5f;
            m_lfoDepthsPresets[ i ][ b ] = 0.5f;
            m_lfoOffsetsPresets[ i ][ b ] = 0.5f;
            m_delayTimesPresets[ i ][ b ] = 0.5f;
            m_feedbacksPresets[ i ][ b ] = 0.5f;
            m_delayMixPresets[ i ][ b ] = 0.5f;
        }
        
        m_polarites[ b ] = false;
        m_delaysOnOff[ b ] = false;
        m_lfosOnOff[ b ] = false;
    }
    
    DBG( "Finished Initialisation" );
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
    initialiseDCBlock( sampleRate );
    
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
        auto r = ( 0.01f * std::pow( 2000.0f, m_lfoRates[ b ] ) );
        m_lfos[ b ].setRateChange( 1.0f/r );
        m_lfos[ b ].setOffset( sjf_scale<float>(0, 1, -1, 1, m_lfoOffsets[ b ] ) );
        m_lfos[ b ].setLFOtype( sjf_lfo::lfoType::sine );
        int lfotyp = *lfoTypeParameter;
        if ( lfotyp == 2 ){ lfotyp = sjf_lfo::lfoType::noise2; }
        m_lfos[ b ].setLFOtype( lfotyp );
        
        lfoDepths[ b ] = std::sqrt(m_lfoDepths[ b ]) * 5.0f;
        bandGainTargets[ b ] = m_bandGains[ b ];
        
        // a little bit of scaling just to keep delay reasonable
        for ( int c = 0; c < 2; c++ )
        {
            delayTimeTarget[ b ] = 1.0f + 0.1f * m_delayTimes[ b ] * getSampleRate();
            delayTimeTarget[ b ] += delayTimeTarget[ b ] * sjf_scale<float>( rand01(), 0.0f, 1.0f, -0.2, 0.2 ); // random fluctuations to add a little bit of spice
        }
        feedbackTarget[ b ] = m_feedbacks[ b ] * 0.999f;
        
        polarityFlip[ b ] = m_polarites[ b ];
        
        lfoOn[ b ] = m_lfosOnOff[ b ];
        delayOn[ b ] = m_delaysOnOff[ b ];
        
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
    std::array< float, NUM_BANDS > filteredAudio, lfoOutputs, delayed, feedback, delayWetMix, delayDryMix, delayDryMixSmoothed, delayWetMixSmoothed,  bandGains;
    
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        delayWetMix[ b ] = std::sqrt( m_delayMix[ b ] );
        delayDryMix[ b ] = std::sqrt( 1.0f - m_delayMix[ b ] );
    }
    
    float sampIn, sampOut, gain, lfoOut;
    
    int whichBands = *bandsParameter;
    int bandStart = (whichBands == 3) ? 1 : 0;
    int bandIncrement = (whichBands == 1) ? 1 : 2;
    for ( int indexThroughBuffer = 0; indexThroughBuffer < bufferSize; indexThroughBuffer++ )
    {
        for ( int b = 0; b < NUM_BANDS; b++ )
        {
            lfoOut = fFold<float > ( m_lfos[ b ].output() * lfoDepths[ b ], -2.0f, 2.0f );
            lfoOutputs[ b ] = m_lfoSmoother[ b ].filterInput( lfoOut );
            for ( int channel = 0; channel < 2; channel++ )
            {
                m_delayLines[ channel ][ b ].setDelayTimeSamps( m_delaySmoother[ b ].filterInput( delayTimeTarget[ b ] ) );
            }
            feedback[ b ] = m_fbSmoother[ b ].filterInput( feedbackTarget[ b ] );
            delayWetMixSmoothed[ b ] = m_delayWetSmoother[ b ].filterInput( delayWetMix[ b ] );
            delayDryMixSmoothed[ b ] = m_delayDrySmoother[ b ].filterInput( delayDryMix[ b ] );
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
                if ( delayOn[ b ] )
                {
                    sampOut += ( delayed[ b ] * delayWetMixSmoothed[ b ] ) + ( filteredAudio[ b ] * delayDryMixSmoothed[ b ] );
                }
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
        bandGainParameter[ b ].setValue( m_bandGains[ b ] );
        polarityParameter[ b ].setValue( m_polarites[ b ] );

        lfosOnOffParameter[ b ].setValue( m_lfosOnOff[ b ] );
        lfoRateParameter[ b ].setValue( m_lfoRates[ b ] );
        lfoDepthParameter[ b ].setValue( m_lfoDepths[ b ] );
        lfoOffsetParameter[ b ].setValue( m_lfoOffsets[ b ] );

        delayTimeParameter[ b ].setValue( m_delayTimes[ b ] );
        feedbackParameter[ b ].setValue( m_feedbacks[ b ] );
        delayMixParameter[ b ].setValue( m_delayMix[ b ] );
        delaysOnOffParameter[ b ].setValue( m_delaysOnOff[ b ] );
    }
    
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        for ( int i = 0; i < m_bandGainsPresets.size(); i++ )
        {
            bandGainPresetsParameter[ i ][ b ].setValue( m_bandGainsPresets[ i ][ b ] );
            
            lfoRatePresetsParameter[ i ][ b ].setValue( m_lfoRatesPresets[ i ][ b ] );
            lfoDepthPresetsParameter[ i ][ b ].setValue( m_lfoDepthsPresets[ i ][ b ] );
            lfoOffsetPresetsParameter[ i ][ b ].setValue( m_lfoOffsetsPresets[ i ][ b ] );
            
            delayTimePresetsParameter[ i ][ b ].setValue( m_delayTimesPresets[ i ][ b ] );
            feedbackPresetsParameter[ i ][ b ].setValue( m_feedbacksPresets[ i ][ b ] );
            delayMixPresetsParameter[ i ][ b ].setValue( m_delayMixPresets[ i ][ b ] );
        }
    }
    auto state = parameters.copyState();
    
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
    
    DBG( "Finished get state" );
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
                for ( int i = 0; i < m_bandGainsPresets.size(); i++ )
                {
                    bandGainPresetsParameter[ i ][ b ].referTo( parameters.state.getPropertyAsValue( "bandGain"+juce::String(i)+"_"+juce::String( b ), nullptr ) );
                    lfoRatePresetsParameter[ i ][ b ].referTo( parameters.state.getPropertyAsValue( "lfoRate"+juce::String(i)+"_"+juce::String( b ), nullptr ) );
                    lfoDepthPresetsParameter[ i ][ b ].referTo( parameters.state.getPropertyAsValue( "lfoDepth"+juce::String(i)+"_"+juce::String( b ), nullptr ) );
                    lfoOffsetPresetsParameter[ i ][ b ].referTo( parameters.state.getPropertyAsValue( "lfoOffset"+juce::String(i)+"_"+juce::String( b ), nullptr ) );
                    
                    delayTimePresetsParameter[ i ][ b ].referTo( parameters.state.getPropertyAsValue("delayTime"+juce::String(i)+"_"+juce::String( b ), nullptr) );
                    feedbackPresetsParameter[ i ][ b ].referTo( parameters.state.getPropertyAsValue("feedback"+juce::String(i)+"_"+juce::String( b ), nullptr) );
                    delayMixPresetsParameter[ i ][ b ].referTo( parameters.state.getPropertyAsValue("delayMix"+juce::String(i)+"_"+juce::String( b ), nullptr) );
                    
                }
            }
            
            for ( int b = 0; b < NUM_BANDS; b++ )
            {
                bandGainParameter[ b ].referTo( parameters.state.getPropertyAsValue( "bandGain"+juce::String( b ), nullptr ) );
                polarityParameter[ b ].referTo( parameters.state.getPropertyAsValue( "polarity"+juce::String( b ), nullptr ) );
                lfoRateParameter[ b ].referTo( parameters.state.getPropertyAsValue( "lfoRate"+juce::String( b ), nullptr ) );
                lfoDepthParameter[ b ].referTo( parameters.state.getPropertyAsValue( "lfoDepth"+juce::String( b ), nullptr ) );
                lfoOffsetParameter[ b ].referTo( parameters.state.getPropertyAsValue( "lfoOffset"+juce::String( b ), nullptr ) );
                
                delayTimeParameter[ b ].referTo( parameters.state.getPropertyAsValue("delayTime"+juce::String( b ), nullptr) );
                feedbackParameter[ b ].referTo( parameters.state.getPropertyAsValue("feedback"+juce::String( b ), nullptr) );
                delayMixParameter[ b ].referTo( parameters.state.getPropertyAsValue("delayMix"+juce::String( b ), nullptr) );
                
                delaysOnOffParameter[ b ].referTo( parameters.state.getPropertyAsValue("delayOnOff"+juce::String( b ), nullptr) );
                lfosOnOffParameter[ b ].referTo( parameters.state.getPropertyAsValue("lfoOnOff"+juce::String( b ), nullptr) );
            }
            
            for ( int b = 0; b < NUM_BANDS; b++ )
            {
                m_bandGains[ b ] = (float)bandGainParameter[ b ].getValue();
                m_polarites[ b ] =  (bool)polarityParameter[ b ].getValue();
                m_lfoRates[ b ] = (float)lfoRateParameter[ b ].getValue();
                m_lfoDepths[ b ] =  (float)lfoDepthParameter[ b ].getValue();
                m_lfoOffsets[ b ] = (float)lfoOffsetParameter[ b ].getValue();
                
                m_delayTimes[ b ] = (float)delayTimeParameter[ b ].getValue();
                m_feedbacks[ b ] = (float)feedbackParameter[ b ].getValue();
                m_delayMix[ b ] = (float)delayMixParameter[ b ].getValue();
                
                m_delaysOnOff[ b ] = (bool)delaysOnOffParameter[ b ].getValue();
                m_lfosOnOff[ b ] = (bool)lfosOnOffParameter[ b ].getValue();
            }
            
            for ( int b = 0; b < NUM_BANDS; b++ )
            {
                for ( int i = 0; i < m_bandGainsPresets.size(); i++ )
                {
                    m_bandGainsPresets[ i ][ b ] = (float)bandGainPresetsParameter[ i ][ b ].getValue();
                    m_lfoRatesPresets[ i ][ b ] = (float)lfoRatePresetsParameter[ i ][ b ].getValue();
                    m_lfoDepthsPresets[ i ][ b ] =  (float)lfoDepthPresetsParameter[ i ][ b ].getValue();
                    m_lfoOffsetsPresets[ i ][ b ] = (float)lfoOffsetPresetsParameter[ i ][ b ].getValue();
                    
                    m_delayTimesPresets[ i ][ b ] = (float)delayTimePresetsParameter[ i ][ b ].getValue();
                    m_feedbacksPresets[ i ][ b ] = (float)feedbackPresetsParameter[ i ][ b ].getValue();
                    m_delayMixPresets[ i ][ b ] = (float)delayMixPresetsParameter[ i ][ b ].getValue();
                    
                }
            }
        }
        
        m_parametersChangedFlag = true;
        
        DBG( "Finished set state" );
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
        m_delayWetSmoother[ b ].setCutoff( calculateLPFCoefficient<float>( 0.1f, (float)sampleRate ) );
        m_delayDrySmoother[ b ].setCutoff( calculateLPFCoefficient<float>( 0.1f, (float)sampleRate ) );
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
void Sjf_spectralProcessorAudioProcessor::initialiseDCBlock( double sampleRate )
{
    float dcCutoff = calculateLPFCoefficient< float > ( 15, getSampleRate() );
    for ( int c = 0; c < dcFilter.size(); c++ )
    {
        dcFilter[ c ].setCutoff( dcCutoff );
    }
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setBandGain( const int bandNumber, const double gain )
{
    m_bandGains[ bandNumber ] = gain;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getBandGain( const int bandNumber )
{
    return m_bandGains[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setBandPolarity( const int bandNumber, const bool flip )
{
    m_polarites[ bandNumber ] = flip;
}
//==============================================================================
const bool Sjf_spectralProcessorAudioProcessor::getBandPolarity( const int bandNumber )
{
    return m_polarites[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFORate( const int bandNumber, const double lfoR )
{
    m_lfoRates[ bandNumber ] = lfoR;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFORate( const int bandNumber )
{
    return m_lfoRates[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFODepth( const int bandNumber, const double lfoD )
{
    m_lfoDepths[ bandNumber ] = lfoD;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFODepth( const int bandNumber )
{
    return m_lfoDepths[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFOOffset( const int bandNumber, const double lfoOffset )
{
    m_lfoOffsets[ bandNumber ] = lfoOffset;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFOOffset( const int bandNumber )
{
    return m_lfoOffsets[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setDelayTime( const int bandNumber, const double delay )
{
    m_delayTimes[ bandNumber ] = delay;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getDelayTime( const int bandNumber )
{
    return m_delayTimes[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setFeedback( const int bandNumber, const double fb )
{
    m_feedbacks[ bandNumber ] = fb;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getFeedback( const int bandNumber )
{
    return m_feedbacks[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setDelayMix( const int bandNumber, const double delayMix )
{
    m_delayMix[ bandNumber ] = delayMix;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getDelayMix( const int bandNumber )
{
    return m_delayMix[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setDelayOn( const int bandNumber, const bool delayIsOn )
{
    m_delaysOnOff[ bandNumber ] = delayIsOn;
}
//==============================================================================
const bool Sjf_spectralProcessorAudioProcessor::getDelayOn( const int bandNumber )
{
    return m_delaysOnOff[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLfoOn( const int bandNumber, const bool lfoIsOn )
{
    m_lfosOnOff[ bandNumber ] = lfoIsOn;
}
//==============================================================================
const bool Sjf_spectralProcessorAudioProcessor::getLfoOn( const int bandNumber )
{
    return m_lfosOnOff[ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setBandGain( const int presetNumber, const int bandNumber, const double gain )
{
    m_bandGainsPresets[ presetNumber ][ bandNumber ] = gain;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getBandGain( const int presetNumber, const int bandNumber )
{
    return m_bandGainsPresets[ presetNumber ][ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFORate( const int presetNumber, const int bandNumber, const double lfoR )
{
    m_lfoRatesPresets[ presetNumber ][ bandNumber ] = lfoR;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFORate( const int presetNumber, const int bandNumber )
{
    return m_lfoRatesPresets[ presetNumber ][ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFODepth( const int presetNumber, const int bandNumber, const double lfoD )
{
    m_lfoDepthsPresets[ presetNumber ][ bandNumber ] = lfoD;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFODepth( const int presetNumber, const int bandNumber )
{
    return m_lfoDepthsPresets[ presetNumber ][ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setLFOOffset( const int presetNumber, const int bandNumber, const double lfoOffset )
{
    m_lfoOffsetsPresets[ presetNumber ][ bandNumber ] = lfoOffset;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getLFOOffset( const int presetNumber, const int bandNumber )
{
    return m_lfoOffsetsPresets[ presetNumber ][ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setDelayTime( const int presetNumber, const int bandNumber, const double delay )
{
    m_delayTimesPresets[ presetNumber ][ bandNumber ] = delay;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getDelayTime( const int presetNumber, const int bandNumber )
{
    return m_delayTimesPresets[ presetNumber ][ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setFeedback( const int presetNumber, const int bandNumber, const double fb )
{
    m_feedbacksPresets[ presetNumber ][ bandNumber ] = fb;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getFeedback( const int presetNumber, const int bandNumber )
{
    return m_feedbacksPresets[ presetNumber ][ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::setDelayMix( const int presetNumber, const int bandNumber, const double delayMix )
{
    m_delayMixPresets[ presetNumber ][ bandNumber ] = delayMix;
}
//==============================================================================
const double Sjf_spectralProcessorAudioProcessor::getDelayMix( const int presetNumber, const int bandNumber )
{
    return m_delayMixPresets[ presetNumber ][ bandNumber ];
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::getPreset(const int presetNumber)
{
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        m_bandGains[ b ] = m_bandGainsPresets[ presetNumber ][ b ];
        m_lfoRates[ b ] = m_lfoRatesPresets[ presetNumber ][ b ];
        m_lfoDepths[ b ] = m_lfoDepthsPresets[ presetNumber ][ b ];
        m_lfoOffsets[ b ] = m_lfoOffsetsPresets[ presetNumber ][ b ];
        m_delayTimes[ b ] = m_delayTimesPresets[ presetNumber ][ b ];
        m_feedbacks[ b ] = m_feedbacksPresets[ presetNumber ][ b ];
        m_delayMix[ b ] = m_delayMixPresets[ presetNumber ][ b ];
    }
    m_parametersChangedFlag = true;
}
//==============================================================================
void Sjf_spectralProcessorAudioProcessor::interpolatePresets( std::array< float, 4 > weights )
{
    float total = 0.0f;
    for ( int i = 0; i < weights.size(); i++ ) { total += weights[ i ]; }
    for ( int i = 0; i < weights.size(); i++ ) { weights[ i ] /= total; }
    
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        m_bandGains[ b ] = 0;
        m_lfoRates[ b ] = 0;
        m_lfoDepths[ b ] = 0;
        m_lfoOffsets[ b ] = 0;
        m_delayTimes[ b ] = 0;
        m_feedbacks[ b ] = 0;
        m_delayMix[ b ] = 0;
    }
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        for ( int i = 0; i < weights.size(); i++ )
        {
            m_bandGains[ b ] += m_bandGainsPresets[ i ][ b ]*weights[ i ];
            m_lfoRates[ b ] += m_lfoRatesPresets[ i ][ b ]*weights[ i ];
            m_lfoDepths[ b ] += m_lfoDepthsPresets[ i ][ b ]*weights[ i ];
            m_lfoOffsets[ b ] += m_lfoOffsetsPresets[ i ][ b ]*weights[ i ];
            m_delayTimes[ b ] += m_delayTimesPresets[ i ][ b ]*weights[ i ];
            m_feedbacks[ b ] += m_feedbacksPresets[ i ][ b ]*weights[ i ];
            m_delayMix[ b ] += m_delayMixPresets[ i ][ b ]*weights[ i ];
        }
    }
    m_parametersChangedFlag = true;
}
//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout Sjf_spectralProcessorAudioProcessor::createParameterLayout()
{
    static constexpr int pIDVersionNumber = 1;
    juce::AudioProcessorValueTreeState::ParameterLayout params;
    
//    filterOrder
    params.add( std::make_unique<juce::AudioParameterInt>( juce::ParameterID{ "lfoType", pIDVersionNumber }, "LfoType", 1, 2, 1 ) );
    params.add( std::make_unique<juce::AudioParameterInt>( juce::ParameterID{ "bands", pIDVersionNumber }, "Bands", 1, 3, 1 ) );
    params.add( std::make_unique<juce::AudioParameterInt>( juce::ParameterID{ "filterDesign", pIDVersionNumber }, "FilterDesign", 1, 3, 1 ) );
    params.add( std::make_unique<juce::AudioParameterInt>( juce::ParameterID{ "filterOrder", pIDVersionNumber }, "FilterOrder", 2, 8, 4 ) );
    
    params.add( std::make_unique<juce::AudioParameterFloat>( juce::ParameterID{ "xyPad-X", pIDVersionNumber }, "XyPad-X", 0, 1, 0 ) );
    params.add( std::make_unique<juce::AudioParameterFloat>( juce::ParameterID{ "xyPad-Y", pIDVersionNumber }, "XyPad-Y", 0, 1, 0 ) );
    
    return params;
}
//==============================================================================
// This creates new instances of the plugin..
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new Sjf_spectralProcessorAudioProcessor();
}
