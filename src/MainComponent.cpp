#include "MainComponent.h"
#include "LookAndFeel.h"

MainComponent::MainComponent()
{
    // Apply custom dark Pro Tools style LookAndFeel
    setLookAndFeel(&DeepDAWLookAndFeel::getInstance());

    // Initialize audio device with sensible real-time defaults
    deviceManager.initialiseWithDefaultDevices(2, 2);

    transport = std::make_unique<TransportComponent>(deviceManager);
    addAndMakeVisible(transport.get());

    setSize(1200, 700);
}

void MainComponent::paint(juce::Graphics& g)
{
    g.fillAll(juce::Colour(0xff1e1e1e)); // Deep dark background
}

void MainComponent::resized()
{
    auto area = getLocalBounds();
    transport->setBounds(area.removeFromTop(48));
    // Remaining area reserved for track list + timeline in later phases
}