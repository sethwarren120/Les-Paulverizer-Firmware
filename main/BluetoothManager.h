#include <Arduino.h>
#include <ArduinoBLE.h>

class BluetoothManager 
{
private:

    static void blePeripheralConnectHandler(BLEDevice central);
    static void blePeripheralDisconnectHandler(BLEDevice central);

    static void midiCommand(byte cmd, byte note, byte velocity);

public:

    static void setup();
    static void loop();

    static void playMidi(int note, int velocity);
};