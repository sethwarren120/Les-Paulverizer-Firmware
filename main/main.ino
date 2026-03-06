#include "BluetoothManager.h"
#include "TrackButton.h"
#include "TrackControls.h"
#include "PlayMidi.h"




PlayMidi* midiTrack1 = new PlayMidi();
TrackAction* trackact = midiTrack1;


TrackButton trackButton1(35, trackact);


// ActionDial[] dials = {
  
// }



void setup() {
  delay(3000);

  Serial.begin(9600);

  Serial.println("Startup");

  BluetoothManager::setup();

  // For testing, Midi tracks are being set up here
  int notes[100] = { 40, 50, 60 };
  int velocities[100] = { 100, 100, 100 };
  int startTimes[100] = { 1000, 2000, 3000 };
  int lengths[100] = { 500, 500, 500 };

  midiTrack1->notes = notes;
  midiTrack1->velocities = velocities;
  midiTrack1->startTimes = startTimes;
  midiTrack1->lengths = lengths;
}

void loop() {
  BluetoothManager::loop();

  trackButton1.loop();
}