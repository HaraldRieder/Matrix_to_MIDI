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

int t_127 = 5/*ms*/; 

const int t_max = 1000/*ms*/; // from t_max on all velocities are minimal and constant  
byte velocities[t_max];

void initVelocities() {
  float nominator = 127.0 * t_127;
  for (int t = 0; t < t_max; t++) {
    float velocity;
    if (t <= t_127)
      velocity = 127.0;
    else 
      velocity = nominator/t;
    velocity = max(velocity,10.0);
    velocities[t] = (byte)(velocity + 0.5);
  }
}
  
