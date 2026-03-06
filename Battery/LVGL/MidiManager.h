

// Midi Manager is a centralized location to handle Midi inputs and outputs
// Code in this class handles things such as different messaging over USB vs Bluetooth, so
// that other code can focus strictly on music functionality
class MidiManager
{
private:

    // First array is track number
    static unsigned int noteTimes[1][128];

public:

    static bool bluetoothConnected;
    static bool usbConnected;

    // Plays a note for a given amount of time
    // The track parameter exists to allow output to different tracks in the future (ie for different instruments)
    // It is unused for now but still there to prevent preexisting code from breaking upon implementation
    static void PlayNoteTimed(int note, int velocity, int length, int track = 0);

    static void PlayNote(int note, int velocity, int track = 0);

    static void EndNote(int note, int track = 0);

    static void loop();
};