#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
PluginProcessor::PluginProcessor()
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       apvts (*this, nullptr, "Parameters", createParameterLayout())
{
}

PluginProcessor::~PluginProcessor()
{
}

// Layout de Parâmetros (Controles do Delay)
juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    juce::AudioProcessorValueTreeState::ParameterLayout layout;

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "delayTime", 1 }, "Delay Time", 
        juce::NormalisableRange<float> (1.0f, 2000.0f, 1.0f), 300.0f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "feedback", 1 }, "Feedback", 
        juce::NormalisableRange<float> (0.0f, 0.95f, 0.01f), 0.4f));

    layout.add (std::make_unique<juce::AudioParameterFloat> (
        juce::ParameterID { "mix", 1 }, "Mix", 
        juce::NormalisableRange<float> (0.0f, 1.0f, 0.01f), 0.5f));

    return layout;
}

//==============================================================================
const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool PluginProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return "Default";
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

//==============================================================================
void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);

    // Reserva 5 segundos de memória para o buffer do delay
    auto maxDelayInSeconds = 5.0;
    auto bufferLength = static_cast<int>(sampleRate * maxDelayInSeconds);

    delayBuffer.setSize (getTotalNumInputChannels(), bufferLength);
    delayBuffer.clear();

    writePosition = 0;
}

void PluginProcessor::releaseResources()
{
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}

//==============================================================================
// LÓGICA DO DELAY (BUFFER CIRCULAR)
void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);
    juce::ScopedNoDenormals noDenormals;

    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    // Carrega os valores atuais dos controles da interface
    float delayTimeMs = apvts.getRawParameterValue ("delayTime")->load();
    float feedback    = apvts.getRawParameterValue ("feedback")->load();
    float mix         = apvts.getRawParameterValue ("mix")->load();

    int numSamples = buffer.getNumSamples();
    int delayBufferLength = delayBuffer.getNumSamples();

    // Converte milissegundos em amostras de áudio
    int delayInSamples = static_cast<int> ((delayTimeMs / 1000.0f) * getSampleRate());

    for (int channel = 0; channel < totalNumInputChannels; ++channel)
    {
        auto* channelData = buffer.getWritePointer (channel);
        auto* delayData   = delayBuffer.getWritePointer (channel);

        int localWritePos = writePosition;

        for (int sample = 0; sample < numSamples; ++sample)
        {
            const float cleanSample = channelData[sample];

            // Calcula onde ler no passado
            int readPosition = localWritePos - delayInSamples;
            if (readPosition < 0)
                readPosition += delayBufferLength;

            float delayedSample = delayData[readPosition];

            // Escreve a entrada atual + feedback no buffer
            delayData[localWritePos] = cleanSample + (delayedSample * feedback);

            // Mistura som limpo (Dry) com atrasado (Wet)
            channelData[sample] = (cleanSample * (1.0f - mix)) + (delayedSample * mix);

            localWritePos++;
            if (localWritePos >= delayBufferLength)
                localWritePos = 0;
        }
    }

    writePosition += numSamples;
    writePosition %= delayBufferLength;
}

//==============================================================================
bool PluginProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

//==============================================================================
void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));
    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts.state.getType()))
            apvts.replaceState (juce::ValueTree::fromXml (*xmlState));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
