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
  Serial.begin(9600);

  Serial.println("Startup");

  digitalWrite(LED_BUILTIN, LOW);

  BluetoothManager::setup();

  int notes[100] = { 40, 50, 60 };
  int velocities[100] = { 100, 100, 100 };
  int startTimes[100] = { 1000, 2000, 3000 };
  int lengths[100] = { 500, 500, 500 };
}

void loop() {
  BluetoothManager::loop();

  trackButton1.loop();
}