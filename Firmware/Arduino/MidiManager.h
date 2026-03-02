

// Midi Manager is a centralized location to handle Midi inputs and outputs
// Code in this class handles things such as different messaging over USB vs Bluetooth, so
// that other code can focus strictly on music functionality
class MidiManager
{
private:

    // If true, we are connecting with USB, if not we are using bluetooth
    static bool usingUSB = false;

    // First array is track number
    static unsigned int noteTimes[1][128];

public:

    // Plays a note for a given amount of time
    // The track parameter exists to allow output to different tracks in the future (ie for different instruments)
    // It is unused for now but still there to prevent preexisting code from breaking upon implementation
    static void PlayNoteTimed(int note, int velocity, int length, int track = 0) {
        PlayNote(note, velocity, track);
        noteTimes[track][note] = millis() + length;
    }

    static void PlayNote(int note, int velocity, int track = 0) {
        // TODO: Implement
    }

    static void EndNote(int note, int track = 0) {
        PlayNote(note, 0, track);
    }

    static void loop() {
        for (int t = 0; t < (sizeof(noteTimes) / sizeof(noteTimes[0])); t++) {
            for (int i = 0; i < 128; i++) {
                if (noteTimes[t][i] != 0 && noteTimes[t][i] < millis()) {
                    noteTimes[t][i] = 0;
                    EndNote(i, t);
                }
            }
        }
    }
};