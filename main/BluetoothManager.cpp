
#include "BluetoothManager.h"
#include <Arduino.h>
#include <ArduinoBLE.h>

    // We again are leaving the code set up for eventual multi-track functionality
    static unsigned int midiState[1][128];
    static bool stateFlag[1][128];

    static BLEService midiService;
    static BLECharacteristic midiCharacteristic;

void BluetoothManager::setup() {
    midiService = BLEService("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");
    midiCharacteristic = BLECharacteristic("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLEWrite | BLEWriteWithoutResponse | BLENotify | BLERead, 5);

        // initialize BLE:
    if (!BLE.begin()) {
        Serial.println("Starting BLE failed!");
        while (1);
    }

    // set local name and advertised service for BLE:
    BLE.setLocalName("Les Paulverizer");
    //BLE.setAdvertisedServiceUuid(midiService.uuid());
    BLE.setAdvertisedService(midiService);

    // add the characteristic and service:
    midiService.addCharacteristic(midiCharacteristic);
    BLE.addService(midiService);

    // start advertising
    BLE.advertise();
    Serial.println("waiting for connections...");

    // register new connection handler
    BLE.setEventHandler(BLEConnected, blePeripheralConnectHandler);

    // register disconnect handler
    BLE.setEventHandler(BLEDisconnected, blePeripheralDisconnectHandler);
}

void BluetoothManager::blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void BluetoothManager::blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

void BluetoothManager::playMidi(int note, int velocity) {
    midiState[0][note] = velocity;
    stateFlag[0][note] = true;
}

void BluetoothManager::loop() {
  // wait for a BLE central
  BLEDevice central = BLE.central();

  // if a central is connected to the peripheral:
  while (central.connected()) {
    BLE.stopAdvertise();

    for (int i = 0; i < 128; i++) {
        if (stateFlag[0][i]) {
            midiCommand(0x80, i, midiState[0][i]);
        }
    }
  }
  midiCommand(0x0, 0, 0);
}

// send a 3-byte midi message
void BluetoothManager::midiCommand(byte cmd, byte note, byte velocity) {
  byte midiData[] = {0x80, 0x80, cmd, note, velocity};
  // MIDI data goes in the last three bytes of the midiData array:
  midiData[2] = cmd;
  midiData[3] = note;
  midiData[4] = velocity;

  midiCharacteristic.writeValue(midiData, 5);
}