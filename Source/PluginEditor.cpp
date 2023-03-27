/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

//#define NUM_BANDS 16
#define SLIDER_WIDTH 400
#define SLIDER_HEIGHT 120
#define SLIDER_HEIGHT2 SLIDER_HEIGHT/2
#define indent 10
#define textHeight 20
#define boxWidth 80

#define WIDTH SLIDER_WIDTH + boxWidth + indent*3
#define HEIGHT SLIDER_HEIGHT + 5*SLIDER_HEIGHT2 + indent*3 + textHeight*2
//==============================================================================
Sjf_spectralProcessorAudioProcessorEditor::Sjf_spectralProcessorAudioProcessorEditor (Sjf_spectralProcessorAudioProcessor& p, juce::AudioProcessorValueTreeState& vts)
    : AudioProcessorEditor (&p), audioProcessor (p), valueTreeState( vts )
{
    const int NUM_BANDS = audioProcessor.getNumBands();
    setLookAndFeel( &otherLookAndFeel );
    
    addAndMakeVisible( &bandGainsMultiSlider);
    bandGainsMultiSlider.setNumSliders( NUM_BANDS );
    bandGainsMultiSlider.setTooltip( "This sets the gain for each band" );
    bandGainsMultiSlider.sendLookAndFeelChange();
    
    addAndMakeVisible( &polarityFlips );
    polarityFlips.setNumRows( 1 );
    polarityFlips.setNumColumns( NUM_BANDS );
    polarityFlips.setTooltip( "This allows you to flip the polarity of individual bands" );
    polarityFlips.setLookAndFeel( &otherLookAndFeel );
    polarityFlips.sendLookAndFeelChange();
    
    
    addAndMakeVisible( &lfoDepthMultiSlider);
    lfoDepthMultiSlider.setNumSliders( NUM_BANDS );
    lfoDepthMultiSlider.setTooltip( "This sets the depth of modulation for each band" );
    lfoDepthMultiSlider.sendLookAndFeelChange();
    
    addAndMakeVisible( &lfoRateMultiSlider);
    lfoRateMultiSlider.setNumSliders( NUM_BANDS );
    lfoRateMultiSlider.setTooltip( "This sets the rate of modulation for each band" );
    lfoRateMultiSlider.sendLookAndFeelChange();
    
    addAndMakeVisible( &lfoOffsetMultiSlider);
    lfoOffsetMultiSlider.setNumSliders( NUM_BANDS );
    lfoOffsetMultiSlider.setTooltip( "This sets the offset of the modulation from the primary gain value" );
    lfoOffsetMultiSlider.sendLookAndFeelChange();
    
    addAndMakeVisible( &delayTimeMultiSlider );
    delayTimeMultiSlider.setNumSliders( NUM_BANDS );
    delayTimeMultiSlider.setTooltip( "This sets the delay time for each band" );
    delayTimeMultiSlider.sendLookAndFeelChange();
    
    addAndMakeVisible( &feedbackMultiSlider );
    feedbackMultiSlider.setNumSliders( NUM_BANDS );
    feedbackMultiSlider.setTooltip( "This sets the feedback for the delay line of each delayline" );
    feedbackMultiSlider.sendLookAndFeelChange();
    
    for (int b = 0; b < NUM_BANDS; b++)
    {
        bandGainsMultiSlider.setSliderValue(b, audioProcessor.getBandGain(b) );
        polarityFlips.setToggleState( 0, b, audioProcessor.getBandPolarity( b ) );
        lfoDepthMultiSlider.setSliderValue(b, audioProcessor.getLFODepth(b) );
        lfoRateMultiSlider.setSliderValue(b, audioProcessor.getLFORate(b) );
        lfoOffsetMultiSlider.setSliderValue(b, audioProcessor.getLFOOffset(b) );
        
        delayTimeMultiSlider.setSliderValue(b, audioProcessor.getDelayTime(b) );
        DBG("DELAY TIME " << audioProcessor.getDelayTime(b) );
        feedbackMultiSlider.setSliderValue( b, audioProcessor.getFeedback(b) );
    }
    
    addAndMakeVisible( &lfoTypeBox );
    lfoTypeBox.addItem("sine", 1);
    lfoTypeBox.addItem("rand", 2);
    lfoTypeBoxAttachment.reset( new juce::AudioProcessorValueTreeState::ComboBoxAttachment ( valueTreeState, "lfoType", lfoTypeBox ) );
    lfoTypeBox.setTooltip("This allows you to choose between two different types of modulation");
    lfoTypeBox.sendLookAndFeelChange();
    
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
        }
//        bandsChoiceBox.setSelectedId( (int)(rand01()*3) + 1 );
//        lfoTypeBox.setSelectedId( (int)(rand01()*2) + 1);
    };
    
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
    //    tooltipWindow.getObject().setAlpha(0.0f);
    //    tooltipsToggle.setLookAndFeel(&otherLookAndFeel);
    tooltipsToggle.setTooltip(MAIN_TOOLTIP);
    
    addAndMakeVisible(&tooltipLabel);
    tooltipLabel.setVisible( false );
    tooltipLabel.setColour( juce::Label::backgroundColourId, otherLookAndFeel.backGroundColour.withAlpha( 0.85f ) );
    tooltipLabel.setTooltip(MAIN_TOOLTIP);
    
    startTimer( 200 );
    // Make sure that before the constructor has finished, you've set the
    // editor's size to whatever you need it to be.
    setSize ( WIDTH, HEIGHT );
}

Sjf_spectralProcessorAudioProcessorEditor::~Sjf_spectralProcessorAudioProcessorEditor()
{
    setLookAndFeel( nullptr );
}

//==============================================================================
void Sjf_spectralProcessorAudioProcessorEditor::paint (juce::Graphics& g)
{
    // (Our component is opaque, so we must completely fill the background with a solid colour)
//    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));
    
    juce::Rectangle<int> r = { WIDTH, HEIGHT + tooltipLabel.getHeight() };
    sjf_makeBackground< 40 >( g, r );

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText("sjf_spectralProcessor", 0, 0, getWidth(), textHeight, juce::Justification::centred, textHeight );
    g.drawFittedText("gain", bandGainsMultiSlider.getX(), bandGainsMultiSlider.getY(), bandGainsMultiSlider.getWidth(), bandGainsMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    const int NUM_BANDS = audioProcessor.getNumBands();
    const float togW = SLIDER_WIDTH / NUM_BANDS;
    for ( int b = 0; b < NUM_BANDS; b ++ )
    {
        g.drawFittedText(juce::CharPointer_UTF8 ("\xc3\x98"), polarityFlips.getX() + togW * b, polarityFlips.getY(), togW, polarityFlips.getHeight(), juce::Justification::centred, textHeight );
    }
    g.drawFittedText("lfo depth", lfoDepthMultiSlider.getX(), lfoDepthMultiSlider.getY(), lfoDepthMultiSlider.getWidth(), lfoDepthMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("lfo rate", lfoRateMultiSlider.getX(), lfoRateMultiSlider.getY(), lfoRateMultiSlider.getWidth(), lfoRateMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("lfo offset", lfoOffsetMultiSlider.getX(), lfoOffsetMultiSlider.getY(), lfoOffsetMultiSlider.getWidth(), lfoOffsetMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("delay time", delayTimeMultiSlider.getX(), delayTimeMultiSlider.getY(), delayTimeMultiSlider.getWidth(), delayTimeMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("feedback", feedbackMultiSlider.getX(), feedbackMultiSlider.getY(), feedbackMultiSlider.getWidth(), feedbackMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    
    
}

void Sjf_spectralProcessorAudioProcessorEditor::resized()
{
    bandGainsMultiSlider.setBounds( indent, textHeight, SLIDER_WIDTH, SLIDER_HEIGHT);
    polarityFlips.setBounds( bandGainsMultiSlider.getX(), bandGainsMultiSlider.getBottom(), SLIDER_WIDTH, textHeight );
    
    lfoDepthMultiSlider.setBounds( polarityFlips.getX(), polarityFlips.getBottom()+indent, SLIDER_WIDTH, SLIDER_HEIGHT2 );
    lfoRateMultiSlider.setBounds( lfoDepthMultiSlider.getX(), lfoDepthMultiSlider.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    lfoOffsetMultiSlider.setBounds( lfoRateMultiSlider.getX(), lfoRateMultiSlider.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    
    delayTimeMultiSlider.setBounds( lfoOffsetMultiSlider.getX(), lfoOffsetMultiSlider.getBottom()+indent, SLIDER_WIDTH, SLIDER_HEIGHT2 );
    feedbackMultiSlider.setBounds( delayTimeMultiSlider.getX(), delayTimeMultiSlider.getBottom(), SLIDER_WIDTH, SLIDER_HEIGHT2 );
    
    lfoTypeBox.setBounds( lfoDepthMultiSlider.getRight()+indent, lfoDepthMultiSlider.getY(), boxWidth, textHeight );
    bandsChoiceBox.setBounds( lfoTypeBox.getX(), lfoTypeBox.getBottom(), boxWidth, textHeight );
    filterDesignBox.setBounds( bandsChoiceBox.getX(), bandsChoiceBox.getBottom(), boxWidth, textHeight );
    filterOrderNumBox.setBounds( filterDesignBox.getX(), filterDesignBox.getBottom(), boxWidth, textHeight );
    randomAllButton.setBounds( filterOrderNumBox.getX(), filterOrderNumBox.getBottom(), boxWidth, boxWidth );
    
    tooltipsToggle.setBounds( randomAllButton.getX(), HEIGHT - textHeight - indent, boxWidth, textHeight );
    
    tooltipLabel.setBounds( 0, HEIGHT, getWidth(), textHeight*4 );
}


void Sjf_spectralProcessorAudioProcessorEditor::timerCallback()
{
    sjf_setTooltipLabel( this, MAIN_TOOLTIP, tooltipLabel );
    
    const int NUM_BANDS = audioProcessor.getNumBands();
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        audioProcessor.setBandGain( b, bandGainsMultiSlider.fetch( b ) );
        audioProcessor.setBandPolarity( b, polarityFlips.fetch( 1, b ) );
        audioProcessor.setLFORate( b, lfoRateMultiSlider.fetch( b ) );
        audioProcessor.setLFODepth( b, lfoDepthMultiSlider.fetch( b ) );
        audioProcessor.setLFOOffset( b, lfoOffsetMultiSlider.fetch( b ) );
        
        audioProcessor.setDelayTime( b, delayTimeMultiSlider.fetch(b) );
        audioProcessor.setFeedback( b, feedbackMultiSlider.fetch(b) );
    }
}
