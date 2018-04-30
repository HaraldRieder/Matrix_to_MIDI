/*
The MIDI output channel 1..16 (or 0..15) is selected with a binary coding switch.
5 resistors map the digital 4 bit value to an analog voltage. 
*/

const int AnalogMax = 1023; // corresponds to Vref
const int Vref = 5;
const byte n_limits = 15;
const int limits[n_limits] = {
  0.2  * AnalogMax / Vref,
  0.6  * AnalogMax / Vref,
  1.0  * AnalogMax / Vref,
  1.4  * AnalogMax / Vref,
  1.7  * AnalogMax / Vref,
  1.9  * AnalogMax / Vref,
  2.1  * AnalogMax / Vref,
  2.3  * AnalogMax / Vref,
  2.55 * AnalogMax / Vref,
  2.7  * AnalogMax / Vref,
  2.8  * AnalogMax / Vref,
  2.9  * AnalogMax / Vref,
  3.0  * AnalogMax / Vref,
  3.1  * AnalogMax / Vref,
  3.2  * AnalogMax / Vref
};

/**
 * @return switch position as byte value 0..15
 */
byte readCodingSwitchValue(int pin) {
  int value = analogRead(pin);
  Serial.print(value);
  Serial.print(" -> ");
  for (byte i = 0 ; i < n_limits; i++) {
    if (value < limits[i])
      Serial.println(i);
      return i;
  }
  Serial.println(n_limits);
  return n_limits;
}

