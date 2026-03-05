class ActionButton 
{
public:

    int pin;

    // TODO: LED PINS FOR VISUAL FEEDBACK

    // Sets the action that this button will do to the selected track
    // This needs to be a function as the action class needs to be updated on hardware
    // references such as LED pins so that it can give external feedback
    void SetTrackAction(ButtonAction* newAction) {
        action = newAction;
        action->SetHardwareReferences(this);
    }

    void loop(int selectedTrack) {
        if (digitalRead(pin)) {
            if (previousState)
                action->HandleHold(selectedTrack);
            else
                action->HandlePush(selectedTrack);

            previousState = true;
        }
        else {
            if (previousState)
                action->HandleRelease(selectedTrack);

            previousState = false;
        }

        action->loop(selectedTrack);
    }

private:

    bool previousState;

    ButtonAction *action;
};



class ButtonAction
{
public:

    // Button Actions will be fully responsible for managing each button press,
    // as they are highly functional buttons
    virtual void HandlePush(int selectedTrack) = 0;
    virtual void HandleRelease(int selectedTrack) = 0;

    virtual void HandleHold(int selectedTrack) = 0;

    virtual void loop(int selectedTrack) = 0;

    void SetHardwareReferences(ActionButton* actionButton) {
        // TODO: Set the LED PIN pointer to reference the LED pins from actionButton
    }

private:

    // TODO: LED PIN pointer

    void SetRGB(int r, int g, int b) {

    }
};



class ActionDial
{
public:

    int pin;

    void SetDialAction(DialAction* newAction) {
        action = newAction;
    }

    void loop(int selectedTrack) {
        int adc = analogRead(pin);

        if (adc != previousAdc)
            action->Update(adc, selectedTrack);

        action->loop(selectedTrack);
        
        previousAdc = adc;
    }

private:

    int previousAdc;

    DialAction *action;
};



class DialAction
{
public:
    
    virtual void Update(int adc, int selectedTrack) = 0;

    virtual void loop(int selectedTrack) = 0;
};