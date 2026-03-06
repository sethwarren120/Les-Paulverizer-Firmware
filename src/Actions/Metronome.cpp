#include <Arduino.h>
#include "TrackButton.h"
#include "MidiManager.h"

// Plays a basic midi track

class PlayMidi : public TrackAction 
{
private:

    bool on = false;

    unsigned int startTime = 0;

    unsigned int tickNum = 0;

    unsigned int bpm;

public:

    void Start() {
        on = true;
        tickNum = 0;
        startTime = millis();
    }

    void End() {
        on = false;
    }

    void loop() {
        if (!on) return;

        unsigned int currTime = millis();

        if ((millis() - startTime) / (3600 / bpm) > tickNum) {
            // TODO: Tick
        }
    }

    // We require a function so that we don't try to play lots of ticks at once or wait a
    // long time for the new ticks to start
    void SetBPM(int newBpm) {
        if (on) {
            End();
            bpm = newBpm;
            Start();
        }
        else {
            bpm = newBpm;
        }
    }
};