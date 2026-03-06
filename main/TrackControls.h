#pragma once

#include "TrackControlsActions.h"

class ActionButton 
{
public:

    int pin;

    int ledPin;

    // TODO: LED PINS FOR VISUAL FEEDBACK

    // Sets the action that this button will do to the selected track
    // This needs to be a function as the action class needs to be updated on hardware
    // references such as LED pins so that it can give external feedback
    void SetTrackAction(ButtonAction* newAction) {
        action = newAction;
        action->SetHardwareReferences(pin, ledPin);
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