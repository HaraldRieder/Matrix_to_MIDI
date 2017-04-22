/*
Calc. MIDI velocity proportional to kinetic energy E of pressed key with mass m:

E = 1/2 m v**2

Assumptions 
constant acceleration: a
constant distance between idle and pressed keys: s

s = 1/2 a t**2

How does the energy E depend on the time t needed to press the key down?

v = at     =>     E = 2 m (s/t)**2

Energy is proportional to the square of 1/t.

We want to be able to define a time t1. Above t_1 the MIDI velocity shall be 1.
Below a time t_127 the MIDI velocity shall be 127.

velocity(t) = a/t**2 + b with
velocity(t_1) = 1 and
velocity(t_127) = 127

yields

a = 126 t_1**2 t_127**2 / (t_1**2 - t_127**2)
b = (t_1**2 - 127 t_127**2) / (t_1**2 - t_127**2)

velocity(t) = (126 t_1**2 t_127**2 / t**2 + t_1**2 - 127 t_127**2) / (t_1**2 - t_127**2)

*/

float t_1 = 500/*ms*/; // 500 too high
float t_127 = 7/*ms*/; // 5 too low, 10 too high

const int max_time = 1000/*ms*/; // over max_time all velocities are 1  
byte velocities[max_time];

void initVelocities(float _t_1, float _t_127) {
  float t_1_square = _t_1 * _t_1;
  float t_127_square = _t_127 * _t_127;
  float denominator = t_1_square - t_127_square;
  for (int t = 0; t < max_time; t++) {
    float velocity = (126.0*t_1_square*t_127_square/(t*t) + t_1_square - 127.0*t_127_square) / denominator;
    if (velocity > 127.0)
      velocity = 127.0;
    else if (velocity < 20.0)
      velocity = 20.0;
    velocities[t] = (byte)(velocity + 0.5);
  }
}
  
