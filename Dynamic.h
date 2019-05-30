/*
Fast key pressure:      3 ms =  23.4375 * 128 us
Playing pianissimo: ~ 100 ms = 781.25   * 128 us
*/

//#define QUADRATIC

//const int t_127 =   30; // * 128 us =   3.84 ms  
const int t_127 = 22;
const int t_max = 2000; // * 128 us = 256.00 ms, from t_max on all velocities are minimal and constant  

byte velocities[t_max];
float velocity_min = 1.0;

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
*/
#ifndef QUADRATIC
void initVelocities() {
  float nominator = 127.0 * t_127;
  for (int t = 0; t < t_max; t++) {
    float velocity;
    if (t <= t_127)
      velocity = 127.0;
    else 
      velocity = nominator/t;
    if (velocity < velocity_min)
      velocity = velocity_min;
    velocities[t] = (byte)velocity;
  }
}
#else
/*
Proportional to 1/t**2
*/
void initVelocities() {
  float nominator = 127.0 * t_127;
  for (int t = 0; t < t_max; t++) {
    float velocity;
    if (t <= t_127)
      velocity = 127.0;
    else 
      velocity = nominator/t;
    if (velocity < velocity_min)
      velocity = velocity_min;
    velocities[t] = (byte)velocity;
  }
}
#endif

  
