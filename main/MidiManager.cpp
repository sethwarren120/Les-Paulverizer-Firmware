#include <Arduino.h>

#include "MidiManager.h"
#include "BluetoothManager.h"

    // First array is track number
    static unsigned int noteTimes[1][128];
// Plays a note for a given amount of time
// The track parameter exists to allow output to different tracks in the future (ie for different instruments)
// It is unused for now but still there to prevent preexisting code from breaking upon implementation
void MidiManager::PlayNoteTimed(int note, int velocity, int length, int track) {
    PlayNote(note, velocity, track);
    noteTimes[track][note] = millis() + length;
}

void MidiManager::PlayNote(int note, int velocity, int track) {
    BluetoothManager::playMidi(note, velocity);
}

void MidiManager::EndNote(int note, int track) {
    PlayNote(note, 0, track);
}

void MidiManager::loop() {
    for (int t = 0; t < (sizeof(noteTimes) / sizeof(noteTimes[0])); t++) {
        for (int i = 0; i < 128; i++) {
            if (noteTimes[t][i] != 0 && noteTimes[t][i] < millis()) {
                noteTimes[t][i] = 0;
                EndNote(i, t);
            }
        }
    }
}