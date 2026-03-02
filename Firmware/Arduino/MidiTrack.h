class MidiTrack 
{
public:

    int trackLength = 10000;

// TODO: Convert to consistent data type with MIDI over BLE code

    int notes[100];
    int velocities[100];

    // Times are in ms
    int startTimes[100];
    int length[100];
};