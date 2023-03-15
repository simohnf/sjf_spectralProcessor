/*
  ==============================================================================

    This file contains the basic framework code for a JUCE plugin editor.

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "PluginEditor.h"

#define NUM_BANDS 16
#define SLIDER_WIDTH 400
#define SLIDER_HEIGHT 120
#define SLIDER_HEIGHT2 SLIDER_HEIGHT/2
#define indent 10
#define textHeight 20

#define WIDTH SLIDER_WIDTH + indent*2
#define HEIGHT SLIDER_HEIGHT + 3*SLIDER_HEIGHT2 + indent*4 + textHeight
//==============================================================================
Sjf_spectralProcessorAudioProcessorEditor::Sjf_spectralProcessorAudioProcessorEditor (Sjf_spectralProcessorAudioProcessor& p)
    : AudioProcessorEditor (&p), audioProcessor (p)
{
    setLookAndFeel( &otherLookAndFeel );
    
    addAndMakeVisible( &bandGainsMultiSlider);
    bandGainsMultiSlider.setNumSliders( NUM_BANDS );
    
    
    addAndMakeVisible( &lfoDepthMultiSlider);
    lfoDepthMultiSlider.setNumSliders( NUM_BANDS );
    
    addAndMakeVisible( &lfoRateMultiSlider);
    lfoRateMultiSlider.setNumSliders( NUM_BANDS );
    
    addAndMakeVisible( &lfoOffsetMultiSlider);
    lfoOffsetMultiSlider.setNumSliders( NUM_BANDS );
    
    
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
    g.fillAll (getLookAndFeel().findColour (juce::ResizableWindow::backgroundColourId));

    g.setColour (juce::Colours::white);
    g.setFont (15.0f);
    g.drawFittedText("sjf_spectralProcessor", 0, 0, getWidth(), textHeight, juce::Justification::centred, textHeight );
    g.drawFittedText("gain", bandGainsMultiSlider.getX(), bandGainsMultiSlider.getY(), bandGainsMultiSlider.getWidth(), bandGainsMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("lfo depth", lfoDepthMultiSlider.getX(), lfoDepthMultiSlider.getY(), lfoDepthMultiSlider.getWidth(), lfoDepthMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("lfo rate", lfoRateMultiSlider.getX(), lfoRateMultiSlider.getY(), lfoRateMultiSlider.getWidth(), lfoRateMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    g.drawFittedText("lfo offset", lfoOffsetMultiSlider.getX(), lfoOffsetMultiSlider.getY(), lfoOffsetMultiSlider.getWidth(), lfoOffsetMultiSlider.getHeight(), juce::Justification::centred, textHeight );
    
    
}

void Sjf_spectralProcessorAudioProcessorEditor::resized()
{
    bandGainsMultiSlider.setBounds( indent, textHeight, SLIDER_WIDTH, SLIDER_HEIGHT);
    lfoDepthMultiSlider.setBounds( bandGainsMultiSlider.getX(), bandGainsMultiSlider.getBottom()+indent, SLIDER_WIDTH, SLIDER_HEIGHT2 );
    lfoRateMultiSlider.setBounds( lfoDepthMultiSlider.getX(), lfoDepthMultiSlider.getBottom()+indent, SLIDER_WIDTH, SLIDER_HEIGHT2 );
    lfoOffsetMultiSlider.setBounds( lfoRateMultiSlider.getX(), lfoRateMultiSlider.getBottom()+indent, SLIDER_WIDTH, SLIDER_HEIGHT2 );
    
}


void Sjf_spectralProcessorAudioProcessorEditor::timerCallback()
{
//    sjf_setTooltipLabel( this, MAIN_TOOLTIP, tooltipLabel );
    
    for ( int b = 0; b < NUM_BANDS; b++ )
    {
        audioProcessor.setBandGain( b, bandGainsMultiSlider.fetch( b ) );
        audioProcessor.setLFORate( b, lfoRateMultiSlider.fetch( b ) );
        audioProcessor.setLFODepth( b, lfoDepthMultiSlider.fetch( b ) );
        audioProcessor.setLFOOffset( b, lfoOffsetMultiSlider.fetch( b ) );
    }
}
