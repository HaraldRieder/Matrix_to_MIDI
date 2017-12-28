/*
Calc. MIDI velocity proportional to velocity of pressed key:

v = at

Assumptions 
constant acceleration during 1 key pressure: a
constant distance between idle and pressed keys: s (may be smaller for black keys)

s = 1/2 a t**2

=> v = 2s/t

At time t_127 the MIDI velocity shall be 127.

velocity(t) = 127 * t_127/t so that velocity(t_127) = 127

Very fast key pressure: 3 ms
Playing pianissimo: ~ 100 ms 
*/

int t_127 = 30; // * 128 us  

const int t_max = 2000; // * 128 us = 256 ms, from t_max on all velocities are minimal and constant  
byte velocities[t_max];

void initVelocities() {
  float nominator = 127.0 * t_127;
  for (int t = 0; t < t_max; t++) {
    float velocity;
    if (t <= t_127)
      velocity = 127.0;
    else 
      velocity = nominator/t;
    velocities[t] = (byte)velocity;
  }
}

/**
 * This function works correctly only if called
 * at least once per ~71.58 minutes.
 * The result will overflow (go back to zero) after ~152.7 hours.
 * 
 * @return current time in units of 128 microseconds
 */
unsigned long _128_micros() {
  static unsigned long last_us = 0;
  static unsigned long major = 0;
  unsigned long us = micros();
  if (us < last_us) {
    major++;
  }
  last_us = us;
  return (us >> 7) + (major << 7);
}
  
