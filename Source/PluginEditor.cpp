/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//#define NUM_BANDS 16
#define SLIDER_WIDTH 496
#define SLIDER_HEIGHT 120
#define SLIDER_HEIGHT2 SLIDER_HEIGHT/2
#define indent 10
#define textHeight 20
#define boxWidth 120

#define WIDTH SLIDER_WIDTH + boxWidth*2 + indent*3
#define HEIGHT SLIDER_HEIGHT + 6*SLIDER_HEIGHT2 + indent*3 + textHeight*4
//==============================================================================
Sjf_spectralProcessorAudioProcessorEditor::Sjf_spectralProcessorAudioProcessorEditor (Sjf_spectralProcessorAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState( vts )
{
    const int NUM_BANDS = audioProcessor.getNumBands();
    setLookAndFeel( &otherLookAndFeel );
    
    //------------------------------------------------------------
    //------------------------------------------------------------
    
    addAndMakeVisible( &bandGainsMultiSlider);
    bandGainsMultiSlider.setNumSliders( NUM_BANDS );
    bandGainsMultiSlider.setTooltip( "This sets the gain for each band" );
    bandGainsMultiSlider.sendLookAndFeelChange();
    bandGainsMultiSlider.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setBandGain( b, bandGainsMultiSlider.fetch( b ) );
        }
        m_canSavePreset = true;
    };
    
    addAndMakeVisible( &polarityFlips );
    polarityFlips.setNumRows( 1 );
    polarityFlips.setNumColumns( NUM_BANDS );
    polarityFlips.setTooltip( "This allows you to flip the polarity of individual bands" );
    polarityFlips.setLookAndFeel( &otherLookAndFeel );
    polarityFlips.sendLookAndFeelChange();
    polarityFlips.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setBandPolarity( b, polarityFlips.fetch( 0, b ) );
        }
        m_canSavePreset = true;
    };
    

    //------------------------------------------------------------
    //------------------------------------------------------------
    addAndMakeVisible( &lfosOnOff );
    lfosOnOff.setNumRows( 1 );
    lfosOnOff.setNumColumns( NUM_BANDS );
    lfosOnOff.setTooltip("This turns the lfos on/off for each band");
    lfosOnOff.setLookAndFeel( &otherLookAndFeel );
    lfosOnOff.sendLookAndFeelChange();
    lfosOnOff.onMouseEvent = [this, NUM_BANDS ]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setLfoOn( b, lfosOnOff.fetch( 0, b ) );
        }
        m_canSavePreset = true;
    };
    
    
    addAndMakeVisible( &lfoDepthMultiSlider);
    lfoDepthMultiSlider.setNumSliders( NUM_BANDS );
    lfoDepthMultiSlider.setTooltip( "This sets the depth of modulation for each band" );
    lfoDepthMultiSlider.sendLookAndFeelChange();
    lfoDepthMultiSlider.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setLFODepth( b, lfoDepthMultiSlider.fetch( b ) );
        }
    };
    
    addAndMakeVisible( &lfoRateMultiSlider);
    lfoRateMultiSlider.setNumSliders( NUM_BANDS );
    lfoRateMultiSlider.setTooltip( "This sets the rate of modulation for each band" );
    lfoRateMultiSlider.sendLookAndFeelChange();
    lfoRateMultiSlider.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setLFORate( b, lfoRateMultiSlider.fetch( b ) );
        }
        m_canSavePreset = true;
    };
    
    addAndMakeVisible( &lfoOffsetMultiSlider);
    lfoOffsetMultiSlider.setNumSliders( NUM_BANDS );
    lfoOffsetMultiSlider.setTooltip( "This sets the offset of the modulation from the primary gain value" );
    lfoOffsetMultiSlider.sendLookAndFeelChange();
    lfoOffsetMultiSlider.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setLFOOffset( b, lfoOffsetMultiSlider.fetch( b ) );
        }
        m_canSavePreset = true;
    };
    
    //------------------------------------------------------------
    //------------------------------------------------------------
    addAndMakeVisible( &delaysOnOff );
    delaysOnOff.setNumRows( 1 );
    delaysOnOff.setNumColumns( NUM_BANDS );
    delaysOnOff.setTooltip("This turns the delaylines on/off for each band");
    delaysOnOff.setLookAndFeel( &otherLookAndFeel );
    delaysOnOff.sendLookAndFeelChange();
    delaysOnOff.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setDelayOn( b, delaysOnOff.fetch( 0, b ) );
        }
        m_canSavePreset = true;
    };
    
    addAndMakeVisible( &delayTimeMultiSlider );
    delayTimeMultiSlider.setNumSliders( NUM_BANDS );
    delayTimeMultiSlider.setTooltip( "This sets the delay time for each band" );
    delayTimeMultiSlider.sendLookAndFeelChange();
    delayTimeMultiSlider.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setDelayTime( b, delayTimeMultiSlider.fetch( b ) );
        }
        m_canSavePreset = true;
    };
    
    addAndMakeVisible( &feedbackMultiSlider );
    feedbackMultiSlider.setNumSliders( NUM_BANDS );
    feedbackMultiSlider.setTooltip( "This sets the feedback for the delay line of each band" );
    feedbackMultiSlider.sendLookAndFeelChange();
    feedbackMultiSlider.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setFeedback( b, feedbackMultiSlider.fetch( b ) );
        }
        m_canSavePreset = true;
    };
    
    addAndMakeVisible( &delayMixMultiSlider );
    delayMixMultiSlider.setNumSliders( NUM_BANDS );
    delayMixMultiSlider.setTooltip( "This sets the wet mix of the delay line of each band" );
    delayMixMultiSlider.sendLookAndFeelChange();
    delayMixMultiSlider.onMouseEvent = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++ )
        {
            audioProcessor.setDelayMix( b, delayMixMultiSlider.fetch( b ) );
        }
        m_canSavePreset = true;
    };
    
    

    //------------------------------------------------------------
    //------------------------------------------------------------
    addAndMakeVisible( &lfoTypeBox );
    lfoTypeBox.addItem("sine", 1);
    lfoTypeBox.addItem("rand", 2);
    lfoTypeBoxAttachment.reset( new juce::AudioProcessorValueTreeState::ComboBoxAttachment ( valueTreeState, "lfoType", lfoTypeBox ) );
    lfoTypeBox.setTooltip("This allows you to choose between two different types of modulation");
    lfoTypeBox.sendLookAndFeelChange();
    
    
    //------------------------------------------------------------
    //------------------------------------------------------------
    
    addAndMakeVisible( &bandsChoiceBox );
    bandsChoiceBox.addItem("all", 1);
    bandsChoiceBox.addItem("odd", 2);
    bandsChoiceBox.addItem("even", 3);
    bandsChoiceBoxAttachment.reset( new juce::AudioProcessorValueTreeState::ComboBoxAttachment ( valueTreeState, "bands", bandsChoiceBox ) );
    bandsChoiceBox.setTooltip("This allows you to choose whether the odd bands (1, 3, 5, etc), the even bands, or both odd and even bands are sent to the output");
    bandsChoiceBox.sendLookAndFeelChange();
    
    
    addAndMakeVisible( &filterDesignBox );
    filterDesignBox.addItem( "butterworth", 1 );
    filterDesignBox.addItem( "bessel", 2 );
    filterDesignBox.addItem( "chebyshev", 3 );
    filterDesignBoxAttachment.reset( new juce::AudioProcessorValueTreeState::ComboBoxAttachment ( valueTreeState, "filterDesign", filterDesignBox ) );
    filterDesignBox.setTooltip( "This allows you to change between different filter designs" );
    filterDesignBox.sendLookAndFeelChange();
    
    addAndMakeVisible( &filterOrderNumBox );
    filterOrderNumBoxAttachment.reset( new juce::AudioProcessorValueTreeState::SliderAttachment ( valueTreeState, "filterOrder", filterOrderNumBox ) );
    filterOrderNumBox.setTooltip("This sets the order for all filters (higher order, steeper roll-off" );
    filterOrderNumBox.sendLookAndFeelChange();
    
    //------------------------------------------------------------
    //------------------------------------------------------------
    addAndMakeVisible( &randomAllButton );
    randomAllButton.setButtonText("random");
    randomAllButton.setTooltip( "This will randomise all of the sliders" );
    randomAllButton.onClick = [this, NUM_BANDS]
    {
        for (int b = 0; b < NUM_BANDS; b++)
        {
            bandGainsMultiSlider.setSliderValue(b, rand01() );
            bool state = rand01() < 0.5 ? true : false;
            polarityFlips.setToggleState(0, b, state );
            lfoDepthMultiSlider.setSliderValue(b, rand01() );
            lfoRateMultiSlider.setSliderValue(b, rand01() );
            lfoOffsetMultiSlider.setSliderValue(b, rand01() );
            
            delayTimeMultiSlider.setSliderValue(b, rand01() );
            feedbackMultiSlider.setSliderValue( b, rand01() );
            delayMixMultiSlider.setSliderValue( b, rand01() );
        }
        bandGainsMultiSlider.onMouseEvent();
        lfoDepthMultiSlider.onMouseEvent();
        lfoRateMultiSlider.onMouseEvent();
        lfoOffsetMultiSlider.onMouseEvent();
        delayTimeMultiSlider.onMouseEvent();
        feedbackMultiSlider.onMouseEvent();
        delayMixMultiSlider.onMouseEvent();
        
        polarityFlips.onMouseEvent();
        delaysOnOff.onMouseEvent();
        lfosOnOff.onMouseEvent();
        
        m_canSavePreset = true;
    };
    
    //------------------------------------------------------------
    //------------------------------------------------------------
    addAndMakeVisible( &tooltipsToggle );
    tooltipsToggle.setButtonText( "Hints" );
    tooltipsToggle.onStateChange = [this]
    {
        if (tooltipsToggle.getToggleState())
        {
            //            tooltipWindow.getObject().setAlpha(1.0f);
            tooltipLabel.setVisible( true );
            setSize (WIDTH, HEIGHT+tooltipLabel.getHeight());
        }
        else
        {
            tooltipLabel.setVisible( false );
            setSize (WIDTH, HEIGHT);
            //            tooltipWindow.getObject().setAlpha(0.0f);
        }
    };
    tooltipsToggle.setTooltip(MAIN_TOOLTIP);
    //------------------------------------------------------------
    //------------------------------------------------------------
    
    addAndMakeVisible(&tooltipLabel);
    tooltipLabel.setVisible( false );
    tooltipLabel.setColour( juce::Label::backgroundColourId, otherLookAndFeel.backGroundColour.withAlpha( 0.85f ) );
    tooltipLabel.setTooltip(MAIN_TOOLTIP);
    //------------------------------------------------------------
    //------------------------------------------------------------
    addAndMakeVisible( &xyPadXSlider );
    xyPadXSliderAttachment.reset( new juce::AudioProcessorValueTreeState::SliderAttachment ( valueTreeState, "xyPad-X", xyPadXSlider ) );
    xyPadXSlider.onValueChange = [this]
    {
        XYpad.setNormalisedXposition( xyPadXSlider.getValue() );
        XYpad.onMouseEvent();
    };
    xyPadXSlider.setSliderStyle( juce::Slider::LinearBar );
    xyPadXSlider.setTextBoxIsEditable( false );
    xyPadXSlider.setTextBoxStyle( juce::Slider:: NoTextBox, true, 0, 0 );
    xyPadXSlider.sendLookAndFeelChange();
    
    addAndMakeVisible( &xyPadYSlider );
    xyPadYSliderAttachment.reset( new juce::AudioProcessorValueTreeState::SliderAttachment ( valueTreeState, "xyPad-Y", xyPadYSlider ) );
    xyPadYSlider.setSliderStyle( juce::Slider::LinearBarVertical );
    xyPadYSlider.onValueChange = [this]
    {
        XYpad.setNormalisedYposition( xyPadYSlider.getValue() );
        XYpad.onMouseEvent();
    };
    xyPadYSlider.setTextBoxStyle( juce::Slider:: NoTextBox, true, 0, 0 );
    xyPadYSlider.sendLookAndFeelChange();
    
    addAndMakeVisible( &XYpad );
    XYpad.shouldDrawCornerCircles( true );
    XYpad.setTooltip( "This allows you to interpolate between the four preset settings" );
    XYpad.onMouseEvent = [this]
    {
        auto  pos = XYpad.getNormalisedPosition();
        if( xyPadXSlider.getValue() != pos[0] ){ xyPadXSlider.setValue( pos[0] ); }
        if( xyPadXSlider.getValue() != pos[1] ){ xyPadYSlider.setValue( pos[1] ); }
        
        
        audioProcessor.interpolatePresets( XYpad.distanceFromCorners() );
        std::array< float, 4 > corners = XYpad.distanceFromCorners();
        DBG( corners[0] << " " << corners[1] << " " << corners[2] << " " << corners[3] );
        m_canSavePreset = false;
        DBG("XYPAD CHANGED!!!!");
    };
    
    
    addAndMakeVisible( &presets );
    presets.setIsRadioGroup( true );
    presets.setNumRows( 1 );
    presets.setNumColumns( 4 );
    presets.setToggleState( 0, 0, true );
    presets.setTooltip("This allows you to store and recall 4 presets for the slider arrays that can then be interpolated between using the XYPad \nWhen you click on a new preset number any changes made will be stored in the previously selected preset, if you make changes and then select the same preset again you will reload the previously saved preset and lose those changes");
    presets.onMouseEvent = [this, NUM_BANDS]
    {
        int newSelection = 0;
        for ( int i = 0; i < presets.getNumButtons(); i++ ) { if( presets.fetch( 0, i ) ){ newSelection = i; } }
        if ( m_canSavePreset && newSelection != m_selectedPreset )
        {
            for ( int b = 0; b < NUM_BANDS; b++ )
            {
                audioProcessor.setBandGain( m_selectedPreset, b, bandGainsMultiSlider.fetch( b ) );
                audioProcessor.setBandPolarity( m_selectedPreset, b, polarityFlips.fetch( 0, b) );
                audioProcessor.setLFODepth( m_selectedPreset, b, lfoDepthMultiSlider.fetch( b ) );
                audioProcessor.setLFORate( m_selectedPreset, b, lfoRateMultiSlider.fetch( b ) );
                audioProcessor.setLFOOffset( m_selectedPreset, b, lfoOffsetMultiSlider.fetch( b ) );
                audioProcessor.setDelayTime( m_selectedPreset, b, delayTimeMultiSlider.fetch(b) );
                audioProcessor.setFeedback( m_selectedPreset, b, feedbackMultiSlider.fetch(b) );
                audioProcessor.setDelayMix( m_selectedPreset, b, delayMixMultiSlider.fetch(b) );
            }
        }
        
        m_selectedPreset = newSelection;
        
        audioProcessor.getPreset( m_selectedPreset );
        std::array< float, 2 > pos;
        switch( m_selectedPreset )
        {
            case 0:
                pos[ 0 ] = pos[ 1 ] = 0;
                break;
            case 1:
                pos[ 0 ] = 1;
                pos[ 1 ] = 0;
                break;
            case 2:
                pos[ 0 ] = 1;
                pos[ 1 ] = 1;
                break;
            case 3:
                pos[ 0 ] = 0;
                pos[ 1 ] = 1;
                break;
        }
        XYpad.setNormalisedPosition( pos );
        xyPadXSlider.setValue( pos[0] );
        xyPadYSlider.setValue( pos[1] );
        
//        m_canSavePreset = true;
    };
    presets.setLookAndFeel( &otherLookAndFeel );
    presets.sendLookAndFeelChange();
    //------------------------------------------------------------
    //------------------------------------------------------------
    
    setParameterValues();
    
    
    startTimer( 200 );
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize ( WIDTH, HEIGHT );
    
    
    audioProcessor.isEditorOpen( true );
    DBG( "Finished constucting interface");
}

Sjf_spectralProcessorAudioProcessorEditor::~Sjf_spectralProcessorAudioProcessorEditor()
{
    setLookAndFeel( nullptr );
    audioProcessor.isEditorOpen( false );
    DBG( "Finished deconstucting interface");
}

//==============================================================================
void Sjf_spectralProcessorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
#ifdef JUCE_DEBUG
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
#else
    juce::Rectangle<int> r = { (int)( WIDTH ), (int)(HEIGHT + tooltipLabel.getHeight()) };
    sjf_makeBackground< 40 >( g, r );
#endif

    
    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText("sjf_spectralProcessor", 0, 0, getWidth(), textHeight, juce::Justification::centred, textHeight );
    g.drawFittedText("gain", bandGainsMultiSlider.getX(), bandGainsMultiSlider.getY(), bandGainsMultiSlider.getWidth(), bandGainsMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    const int NUM_BANDS = audioProcessor.getNumBands();
    const float togW = SLIDER_WIDTH / NUM_BANDS;
    
    
    std::vector< juce::String > frequencies =
    { "<100" , "150", "250", "350", "500", "630", "800", "1k", "1.3k", "1.6k", "2k", "2.6k", "3.5k", "5k", "8k", ">10k" };
    
    for ( int b = 0; b < NUM_BANDS; b ++ )
    {
        g.drawFittedText( frequencies[ b ], 2 + bandGainsMultiSlider.getX() + togW * b, bandGainsMultiSlider.getY(), togW - 4, togW, juce::Justification::centred, textHeight );
        
        g.drawFittedText(juce::CharPointer_UTF8 ("\xc3\x98"), polarityFlips.getX() + togW * b, polarityFlips.getY(), togW, polarityFlips.getHeight(), juce::Justification::centred, textHeight );
    }
    g.drawFittedText("lfo depth", lfoDepthMultiSlider.getX(), lfoDepthMultiSlider.getY(), lfoDepthMultiSlider.getWidth(), lfoDepthMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("lfo rate", lfoRateMultiSlider.getX(), lfoRateMultiSlider.getY(), lfoRateMultiSlider.getWidth(), lfoRateMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("lfo offset", lfoOffsetMultiSlider.getX(), lfoOffsetMultiSlider.getY(), lfoOffsetMultiSlider.getWidth(), lfoOffsetMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("delay time", delayTimeMultiSlider.getX(), delayTimeMultiSlider.getY(), delayTimeMultiSlider.getWidth(), delayTimeMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("feedback", feedbackMultiSlider.getX(), feedbackMultiSlider.getY(), feedbackMultiSlider.getWidth(), feedbackMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("mix", delayMixMultiSlider.getX(), delayMixMultiSlider.getY(), delayMixMultiSlider.getWidth(), delayMixMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    
    g.drawFittedText("presets", presets.getX(), presets.getY(), presets.getWidth(), presets.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("presets", XYpad.getX(), XYpad.getY(), XYpad.getWidth(), XYpad.getHeight(), juce::Justification::centred, textHeight );
    
}

void Sjf_spectralProcessorAudioProcessorEditor::resized()
{
    bandGainsMultiSlider.setBounds( indent, textHeight, SLIDER_WIDTH, SLIDER_HEIGHT);
    polarityFlips.setBounds( bandGainsMultiSlider.getX(), bandGainsMultiSlider.getBottom(), SLIDER_WIDTH, textHeight );
    
    lfosOnOff.setBounds( polarityFlips.getX(), polarityFlips.getBottom()+indent, SLIDER_WIDTH, textHeight );
    lfoDepthMultiSlider.setBounds( lfosOnOff.getX(), lfosOnOff.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    lfoRateMultiSlider.setBounds( lfoDepthMultiSlider.getX(), lfoDepthMultiSlider.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    lfoOffsetMultiSlider.setBounds( lfoRateMultiSlider.getX(), lfoRateMultiSlider.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    
    delaysOnOff.setBounds( lfoOffsetMultiSlider.getX(), lfoOffsetMultiSlider.getBottom()+indent, SLIDER_WIDTH, textHeight );
    delayTimeMultiSlider.setBounds( delaysOnOff.getX(), delaysOnOff.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    feedbackMultiSlider.setBounds( delayTimeMultiSlider.getX(), delayTimeMultiSlider.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    delayMixMultiSlider.setBounds( feedbackMultiSlider.getX(), feedbackMultiSlider.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    
    lfoTypeBox.setBounds( bandGainsMultiSlider.getRight()+indent, bandGainsMultiSlider.getY(), boxWidth, textHeight );
    bandsChoiceBox.setBounds( lfoTypeBox.getX(), lfoTypeBox.getBottom(), boxWidth, textHeight );
    filterDesignBox.setBounds( bandsChoiceBox.getX(), bandsChoiceBox.getBottom(), boxWidth, textHeight );
    filterOrderNumBox.setBounds( filterDesignBox.getX(), filterDesignBox.getBottom(), boxWidth, textHeight );
    
    randomAllButton.setBounds( lfoTypeBox.getRight(), lfoTypeBox.getY(), boxWidth, textHeight*4 );
    
    presets.setBounds( lfoTypeBox.getX(), filterOrderNumBox.getBottom() + indent, boxWidth*2, textHeight );
//    auto xySliderSize = textHeight/2;
    XYpad.setBounds( presets.getX()+ indent, presets.getBottom(), boxWidth*2 - indent, boxWidth*2 - indent );
    xyPadXSlider.setBounds( XYpad.getX(), XYpad.getBottom(), XYpad.getWidth(), indent );
    xyPadYSlider.setBounds( lfoTypeBox.getX(), XYpad.getY(), indent, XYpad.getHeight() );
    
    tooltipsToggle.setBounds( randomAllButton.getX(), HEIGHT - textHeight - indent, boxWidth, textHeight );
    
    tooltipLabel.setBounds( 0, HEIGHT, getWidth(), textHeight*5 );
}


void Sjf_spectralProcessorAudioProcessorEditor::timerCallback()
{
    sjf_setTooltipLabel( this, MAIN_TOOLTIP, tooltipLabel );
    
    if( audioProcessor.checkIfParametersChanged() ) { setParameterValues(); }
    audioProcessor.setParametersChangedFalse();
    
}



void Sjf_spectralProcessorAudioProcessorEditor::setParameterValues()
{
    const int NUM_BANDS = audioProcessor.getNumBands();
    
    for (int b = 0; b < NUM_BANDS; b++)
    {
        bandGainsMultiSlider.setSliderValue(b, audioProcessor.getBandGain(b) );
        polarityFlips.setToggleState( 0, b, audioProcessor.getBandPolarity( b ) );
        
        lfosOnOff.setToggleState( 0, b, audioProcessor.getLfoOn( b ) );
        lfoDepthMultiSlider.setSliderValue(b, audioProcessor.getLFODepth(b) );
        lfoRateMultiSlider.setSliderValue(b, audioProcessor.getLFORate(b) );
        lfoOffsetMultiSlider.setSliderValue(b, audioProcessor.getLFOOffset(b) );
        
        delaysOnOff.setToggleState( 0, b, audioProcessor.getDelayOn( b ) );
        delayTimeMultiSlider.setSliderValue(b, audioProcessor.getDelayTime(b) );
        feedbackMultiSlider.setSliderValue( b, audioProcessor.getFeedback(b) );
        delayMixMultiSlider.setSliderValue( b, audioProcessor.getDelayMix(b) );
    }
}
