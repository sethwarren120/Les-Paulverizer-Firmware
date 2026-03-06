
// MIDI structure could possibly do with a rework

// There is a button that could be used for recording new MIDI tracks
// There is also an SD card slot on the ESP32 we are using that could potentially be 
// used to store midi tracks

struct MidiTrack 
{
public:

    int trackLength = 10000;

    // TODO: Improve MIDI structure

    int notes[100];
    int velocities[100];

    // Times are in ms
    int startTimes[100];
    int length[100];
};