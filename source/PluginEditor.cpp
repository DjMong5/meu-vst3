#include "PluginEditor.h"

PluginEditor::PluginEditor (PluginProcessor& p)
    : AudioProcessorEditor (&p), processorRef (p)
{
    juce::ignoreUnused (processorRef);

    // Configuração do Slider: Delay Time
    delayTimeSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    delayTimeSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(delayTimeSlider);

    delayTimeLabel.setText("Delay Time", juce::dontSendNotification);
    delayTimeLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(delayTimeLabel);

    // Configuração do Slider: Feedback
    feedbackSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    feedbackSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(feedbackSlider);

    feedbackLabel.setText("Feedback", juce::dontSendNotification);
    feedbackLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(feedbackLabel);

    // Configuração do Slider: Mix (Dry/Wet)
    mixSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    mixSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    addAndMakeVisible(mixSlider);

    mixLabel.setText("Mix", juce::dontSendNotification);
    mixLabel.setJustificationType(juce::Justification::centred);
    addAndMakeVisible(mixLabel);

    // Botão de inspeção da Melatonin
    addAndMakeVisible (inspectButton);
    inspectButton.onClick = [&] {
        if (!inspector)
        {
            inspector = std::make_unique<melatonin::Inspector> (*this);
            inspector->onClose = [this]() { inspector.reset(); };
        }
        inspector->setVisible (true);
    };

    // Tamanho total da janela do plugin
    setSize (500, 300);
}

PluginEditor::~PluginEditor()
{
}

void PluginEditor::paint (juce::Graphics& g)
{
    // Fundo escuro
    g.fillAll (juce::Colours::darkgrey);

    // Título do Plugin
    g.setColour (juce::Colours::white);
    g.setFont (20.0f);
    g.drawText ("DELAY PLUGIN", getLocalBounds().removeFromTop(40), juce::Justification::centred, false);
}

void PluginEditor::resized()
{
    auto area = getLocalBounds();
    
    // Espaço para o título e topo
    area.removeFromTop(40);
    
    // Reserva a parte inferior para o botão do inspector
    auto bottomArea = area.removeFromBottom(40);
    inspectButton.setBounds (bottomArea.withSizeKeepingCentre(120, 30));

    // Divide a área central igualmente entre os 3 sliders
    auto sliderWidth = area.getWidth() / 3;

    auto delayArea = area.removeFromLeft(sliderWidth);
    delayTimeLabel.setBounds(delayArea.removeFromTop(20));
    delayTimeSlider.setBounds(delayArea);

    auto feedbackArea = area.removeFromLeft(sliderWidth);
    feedbackLabel.setBounds(feedbackArea.removeFromTop(20));
    feedbackSlider.setBounds(feedbackArea);

    auto mixArea = area;
    mixLabel.setBounds(mixArea.removeFromTop(20));
    mixSlider.setBounds(mixArea);
}
