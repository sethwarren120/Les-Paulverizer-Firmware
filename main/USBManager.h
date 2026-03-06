// USB functionality was originally implemented in the first version of the device
// The library used for the original is incompatible with the new ESP32-S3

class USBManager
{
//     void noteOn(byte channel, byte pitch, byte velocity) {
//   midiEventPacket_t noteOn = {0x09, 0x90 | channel, pitch, velocity};
//   MidiUSB.sendMIDI(noteOn);
// }

// void noteOff(byte channel, byte pitch, byte velocity) {
//   midiEventPacket_t noteOff = {0x08, 0x80 | channel, pitch, velocity};
//   MidiUSB.sendMIDI(noteOff);
// }

// //Checks if button connected to pin button was pressed and sends MIDI signal over USB
// void checkButtonUSB(int button, byte note) {
//   if (digitalRead(button) == HIGH && prevButtonState[note - 1] == 0)
//   {
//     noteOn(1, note, 127); //Sends out midi signal of int note and velocity 127 on channel 1
//     MidiUSB.flush();
//     prevButtonState[note - 1] = 1;
//   } else if (digitalRead(button) == LOW && prevButtonState[note - 1] == 1) {
//     if (bHoldNotToggle) {
//       noteOff(1, note, 127); //Sends out midi signal to turn off the note
//     } else {
//       noteOff(1, note, 0); //Sends out midi signal to turn off the note
//     }
//     MidiUSB.flush();
//     prevButtonState[note - 1] = 0;
//   }
// }

// //Checks if pot connected to pin pot was moved and sends MIDI signal over USB
// void checkPotUSB(int pot, byte note, int curve) {
//   int current;
//   switch(curve) {
//     case LINEAR:
//     {
//       current = analogRead(pot) / 8;
//       break;
//     }
//     case AUDIO:
//     {
//       double temp = analogRead(pot);
//       current = 1024*pow((temp/1024),3);
//       current = temp/8;
//       break;
//     }
//     case INVERSE:
//     {
//       double temp = analogRead(pot);
//       current = 1024*cbrt(temp/1024);
//       current = temp/8;
//     }
//   }
//   if (current != prevPotState[note - 1])
//   {
//     noteOn(182, note, current); //Sends out midi signal of int note and velocity 'current' on channel 176 (CC)
//     MidiUSB.flush();
//     prevPotState[note - 1] = current;
//   }
// }
};