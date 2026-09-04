#include <JuceHeader.h>
#include "MainComponent.h"

class BassAmpApplication final : public juce::JUCEApplication
{
public:
    const juce::String getApplicationName() override { return "Bass_amp"; }
    const juce::String getApplicationVersion() override { return "1.0.0"; }
    bool moreThanOneInstanceAllowed() override { return true; }

    void initialise(const juce::String&) override
    {
        mainWindow = std::make_unique<MainWindow>(getApplicationName());
    }

    void shutdown() override
    {
        mainWindow.reset();
    }

    void systemRequestedQuit() override
    {
        quit();
    }

private:
    class MainWindow final : public juce::DocumentWindow
    {
    public:
        explicit MainWindow(juce::String name)
            : DocumentWindow(std::move(name),
                             juce::Colour::fromRGB(18, 20, 24),
                             DocumentWindow::allButtons)
        {
            setUsingNativeTitleBar(true);
            setResizable(true, false);
            setResizeLimits(980, 650, 1800, 1100);
            setContentOwned(new MainComponent(), true);
            centreWithSize(getWidth(), getHeight());
            setVisible(true);
        }

        void closeButtonPressed() override
        {
            juce::JUCEApplication::getInstance()->systemRequestedQuit();
        }
    };

    std::unique_ptr<MainWindow> mainWindow;
};

START_JUCE_APPLICATION(BassAmpApplication)
