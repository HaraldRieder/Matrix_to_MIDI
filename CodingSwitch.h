/*
The MIDI output channel 1..16 (or 0..15) is selected with a binary coding switch.
5 resistors map the digital 4 bit values to analog voltages. 
*/

const int n_values = 10;
const int values[n_values] = {0,510,311,602,173,558,401,636,90,534};
const int delta = 10;

//#define DEBUG_CODING_SWITCH

/**
 * @return switch position as byte value 0..15
 */
byte readCodingSwitchValue(int pin) {
  int value = analogRead(pin);
  #ifdef DEBUG_CODING_SWITCH
  Serial.print("Coding switch ");
  Serial.print(value);
  Serial.print(" -> ");
  #endif
  for (int i = 0 ; i < n_values; i++) {
    if (value <= values[i] + delta && value >= values[i] - delta) {
      #ifdef DEBUG_CODING_SWITCH
      Serial.println(i);
      #endif
      return i;
    }
  }
  #ifdef DEBUG_CODING_SWITCH
  Serial.println(0);
  #endif
  return 0;
}

