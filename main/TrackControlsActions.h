#pragma once

class ButtonAction
{
public:

    // Button Actions will be fully responsible for managing each button press,
    // as they are highly functional buttons
    virtual void HandlePush(int selectedTrack) = 0;
    virtual void HandleRelease(int selectedTrack) = 0;

    virtual void HandleHold(int selectedTrack) = 0;

    virtual void loop(int selectedTrack) = 0;

    void SetHardwareReferences(int buttonPin, int ledPin) {
        // TODO: Set the LED PIN pointer to reference the LED pins from actionButton
    }

private:

    // TODO: LED PIN pointer

    void SetRGB(int r, int g, int b) {

    }
};

class DialAction
{
public:
    
    virtual void Update(int adc, int selectedTrack) = 0;

    virtual void loop(int selectedTrack) = 0;
};