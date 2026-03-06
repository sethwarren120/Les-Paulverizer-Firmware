#pragma once

// The purpose of the track button class is to allow the functionality of a button to be changed
// Most of the time the button will play a midi track, but it could also be something such as a metronome
// The eventual goal would be to get these to change through the browser based DAW

class TrackAction 
{
public:

    // Track buttons will have a start and end trigger as the experience of using track buttons should be
    // consistent, even when the task each is doing is different
    virtual void Start() = 0;
    virtual void End() = 0;

    virtual void loop() = 0;
};

class TrackButton 
{
public:

    int pin;

    // TODO: LED PINS FOR VISUAL FEEDBACK

    // TODO: Make action assignment a function for consistency with other action classes

    TrackAction *action;

    TrackButton(int digitalPin, TrackAction* buttonAction) {
        pinMode(pin, OUTPUT);
        action = buttonAction;
    }

    void loop() {
        bool state = digitalRead(pin);

        if (state != previousState) {
            if (state)
                action->Start();
            else
                action->End();
        }

        action->loop();

        previousState = state;
    }

private:

    bool previousState;
};



