#include <ArduinoBLE.h>
#include <MIDIUSB.h>

#include "TrackButton.h"
#include "TrackControls.h"



TrackButton[] tracks = {
  
};

ActionButton[] actions = {

}

ActionDial[] dials = {
  
}



//Define Buttons ports to D0-D14
//Do not change pin values
//#define button_zero 0
//#define button_one 1
//#define button_two 2
//#define button_three 3
//#define button_four 4
//#define button_five 5
#define button_six 6
#define button_seven 7
#define button_eight 8
#define button_nine 9
//#define button_ten 10
//#define button_eleven 11
//#define button_twelve 12
//#define button_thirteen 13
//#define button_fourteen 14

//D15-D21 are usable but should be prioritized for poteniometers
//Make sure to not double define with analog pin ports for potentiometers
//#define button_fifteen 15        //A0
//#define button_sixteen 16        //A1
//#define button_seventeen 17      //A2
//#define button_eighteen 18       //A3
//#define button_nineteen 19       //A4
//#define button_twenty 20         //A5
//#define button_twenty_one 21     //A6

//Define MIDI value that button should be assositiated with channel 1
//Change note value to desired MIDI note
//#define button_zero_note 51
//#define button_one_note 53
//#define button_two_note 55
//#define button_three_note 57
//#define button_four_note 59
//#define button_five_note 61
#define button_six_note 63
#define button_seven_note 65
#define button_eight_note 67
#define button_nine_note 69
//#define button_ten_note 71
//#define button_eleven_note 73
//#define button_twelve_note 75
//#define button_thirteen_note 77
//#define button_fourteen_note 79

//Analog Pins Used as buttons
//#define button_fifteen_note 81
//#define button_sixteen_note 83
//#define button_seventeen_note 85
//#define button_eighteen_note 87
//#define button_nineteen_note 89
//#define button_twenty_note 91
//#define button_twenty_one_note 93

//Define Potentiometer ports D15-D21
//Do not change pin value
//#define pot_zero A0
//#define pot_one A1
//#define pot_two A2
#define pot_three A3
//#define pot_four A4
//#define pot_five A5
//#define pot_six A6


//Change to note on midi channel 7 for variable value such as a slider
//#define pot_zero_note 1
//#define pot_one_note 1
//#define pot_two_note 1
#define pot_three_note 1
//#define pot_four_note 1
//#define pot_five_note 1
//#define pot_six_note 1

//Define Potentiometer Curve/taper Corresponding to Choosen pots above (OPTIONS: LINEAR, AUDIO, INVERSE)
#define LINEAR 0
#define AUDIO 1
#define INVERSE 2
//#define pot_zero_curve LINEAR
//#define pot_one_curve LINEAR
//#define pot_two_curve LINEAR
#define pot_three_curve LINEAR
//#define pot_four_curve LINEAR
//#define pot_five_curve LINEAR
//#define pot_six_curve LINEAR


byte midiData[] = {0x80, 0x80, 0x00, 0x00, 0x00};
int prevButtonState[] = {0, 0, 0, 0};
int prevPotState[] = {0};
bool bHoldNotToggle = false;

// set up the MIDI service and MIDI message characteristic:
BLEService midiService("03B80E5A-EDE8-4B33-A751-6CE34EC4C700");
BLECharacteristic midiCharacteristic("7772E5DB-3868-4112-A1A9-F2669D106BF3", BLEWrite | BLEWriteWithoutResponse | BLENotify | BLERead, sizeof(midiData));

void setup() {
  // initialize serial communication
  Serial.begin(9600);

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

  //TODO: Add hold/toggle test here
  if (digitalRead(button_six)) {
    bHoldNotToggle = true;
  }
}

void blePeripheralConnectHandler(BLEDevice central) {
  // central connected event handler
  Serial.print("Connected event, central: ");
  Serial.println(central.address());
}

void blePeripheralDisconnectHandler(BLEDevice central) {
  // central disconnected event handler
  Serial.print("Disconnected event, central: ");
  Serial.println(central.address());
}

void loop() {
  // wait for a BLE central
  BLEDevice central = BLE.central();

  // if a central is connected to the peripheral:
  while (central.connected()) {
    BLE.stopAdvertise();
    //Buttons
  }
  midiCommand(0x0, 0, 0);
}

// send a 3-byte midi message
void midiCommand(byte cmd, byte note, byte velocity) {
  // MIDI data goes in the last three bytes of the midiData array:
  midiData[2] = cmd;
  midiData[3] = note;
  midiData[4] = velocity;

  midiCharacteristic.writeValue(midiData, sizeof(midiData));
}

/*
So because the BT version sends 127 velocity for note on and note off, the web app interprets it as "hold-to-play"
The USB version sends velo 0 for note off so it's a toggle. We now want it to default to toggle for both interfaces.
But if the purple button is held on startup both becomes holds
*/

//Checks if button connected to pin button was pressed and sends MIDI signal over BLE
void checkButtonBLE(int button, byte note) {
  if (digitalRead(button) == HIGH && prevButtonState[note - 1] == 0)
  {
    midiCommand(0x90, note, 127); //Sends note on command
    prevButtonState[note - 1] = 1;
  } else if (digitalRead(button) == LOW && prevButtonState[note - 1] == 1) {
    if (bHoldNotToggle) {
      midiCommand(0x80, note, 127); //Sends note off command
    } else {
      midiCommand(0x80, note, 0); //Sends note off command
    }
    prevButtonState[note - 1] = 0;
  }
}

void noteOn(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOn);
}

void noteOff(byte channel, byte pitch, byte velocity) {
  midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
  MidiUSB.sendMIDI(noteOff);
}

//Checks if button connected to pin button was pressed and sends MIDI signal over USB
void checkButtonUSB(int button, byte note) {
  if (digitalRead(button) == HIGH && prevButtonState[note - 1] == 0)
  {
    noteOn(1, note, 127); //Sends out midi signal of int note and velocity 127 on channel 1
    MidiUSB.flush();
    prevButtonState[note - 1] = 1;
  } else if (digitalRead(button) == LOW && prevButtonState[note - 1] == 1) {
    if (bHoldNotToggle) {
      noteOff(1, note, 127); //Sends out midi signal to turn off the note
    } else {
      noteOff(1, note, 0); //Sends out midi signal to turn off the note
    }
    MidiUSB.flush();
    prevButtonState[note - 1] = 0;
  }
}

//Checks if pot connected to pin pot was moved and sends MIDI signal over USB
void checkPotUSB(int pot, byte note, int curve) {
  int current;
  switch(curve) {
    case LINEAR:
    {
      current = analogRead(pot) / 8;
      break;
    }
    case AUDIO:
    {
      double temp = analogRead(pot);
      current = 1024*pow((temp/1024),3);
      current = temp/8;
      break;
    }
    case INVERSE:
    {
      double temp = analogRead(pot);
      current = 1024*cbrt(temp/1024);
      current = temp/8;
    }
  }
  if (current != prevPotState[note - 1])
  {
    noteOn(182, note, current); //Sends out midi signal of int note and velocity 'current' on channel 176 (CC)
    MidiUSB.flush();
    prevPotState[note - 1] = current;
  }
}

//Checks if pot connected to pin pot was moved and sends MIDI signal over BLE
void checkPotBLE(int pot, byte note, int curve) {
  int current;
  switch(curve) {
    case LINEAR:
    {
      current = analogRead(pot) / 8;
      break;
    }
    case AUDIO:
    {
      double temp = analogRead(pot);
      current = 1024*pow((temp/1024),3);
      current = temp/8;
      break;
    }
    case INVERSE:
    {
      double temp = analogRead(pot);
      current = 1024*cbrt(temp/1024);
      current = temp/8;
    }
  }
  if (current != prevPotState[note - 1])
  {
    midiCommand(0xB6, note, current); //Sends out midi signal of int note and velocity 'current' on channel 7 (CC)
    prevPotState[note - 1] = current;
  }
}
