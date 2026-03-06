#include <Arduino.h>
#include "TrackButton.h"
#include "MidiManager.h"
#include "MidiTrack.h"

// Plays a basic midi track

class PlayMidi : public TrackAction 
{
private:

    bool on = false;

    unsigned int startTime = 0;

    unsigned int noteIndex = 0;

    MidiTrack *track;

public:

    void Start() {
        on = true;
        noteIndex = 0;
        startTime = millis();
    }

    void End() {
        on = false;
    }

    void loop() {
        if (!on) return;

        unsigned int currTime = millis();

        // We will assume that the notes are already arranged in time order
        // For every loop, we will play every note in the queue that has reached its start
        // time since we started this track iteration
        while (track->startTimes[noteIndex] > (currTime - startTime)) {
            MidiManager::PlayNoteTimed(track->notes[noteIndex], track->velocities[noteIndex], track->length[noteIndex]);
            noteIndex++;

            if (noteIndex == sizeof(track->notes) / sizeof(track->notes[0])) {
                noteIndex = 0;
                startTime += track->trackLength;
            }
        }
    }

    // We require a function so that the track is not set while a different track is playing,
    // as that could lead to us indexing into a note beyond the bounds of the new track
    void SetTrack(MidiTrack *newTrack) {
        if (on) {
            End();
            track = newTrack;
            Start();
        }
        else {
            track = newTrack;
        }
    }
};