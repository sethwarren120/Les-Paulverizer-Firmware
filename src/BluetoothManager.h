#include <Arduino.h>
#include <ArduinoBLE.h>

class BluetoothManager 
{
private:

    // We again are leaving the code set up for eventual multi-track functionality
    static unsigned int midiState[1][128];
    static bool stateFlag[1][128];

    static void blePeripheralConnectHandler(BLEDevice central);
    static void blePeripheralDisconnectHandler(BLEDevice central);

    static void midiCommand(byte cmd, byte note, byte velocity);

public:

    static BLEService midiService;
    static BLECharacteristic midiCharacteristic;

    static void setup();
    static void loop();

    static void playMidi(int note, int velocity);
};