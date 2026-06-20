#include "DeepDAWApplication.h"

DeepDAWApplication::MainWindow::MainWindow(const juce::String& name)
    : DocumentWindow(name,
                     juce::Colours::darkgrey,
                     DocumentWindow::allButtons)
{
    setUsingNativeTitleBar(true);
    setContentOwned(new MainComponent(), true);

    setResizable(true, true);
    centreWithSize(1200, 700);
    setVisible(true);
}

void DeepDAWApplication::MainWindow::closeButtonPressed()
{
    JUCEApplication::getInstance()->systemRequestedQuit();
}

const juce::String DeepDAWApplication::getApplicationName()
{
    return "DeepDAW";
}

const juce::String DeepDAWApplication::getApplicationVersion()
{
    return "0.1.0";
}

void DeepDAWApplication::initialise(const juce::String&)
{
    mainWindow.reset(new MainWindow(getApplicationName()));
}

void DeepDAWApplication::shutdown()
{
    mainWindow = nullptr;
}