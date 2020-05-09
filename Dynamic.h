/*
Fast key pressure:      3 ms =  23.4375 * 128 us
Playing pianissimo: ~ 100 ms = 781.25   * 128 us
*/

//#define QUADRATIC

//const int t_127 = 30;   // * 128 us =   3.84 ms  
//const int t_127 = 25;   // * 128 us =   3.20 ms  
const int t_127 = 23;   // * 128 us =   2.94 ms  
const int t_max = 1000; // * 128 us = 128.00 ms, from t_max on all velocities are minimal and constant  

byte velocities[t_max];
float velocity_min = 1.0;

/*
Calc. MIDI velocity proportional to velocity of pressed key:

v = at

=> v ~ 1/t

Experience: too long low velocity values, only a narrow range where all the higher values live
Therefore: v ~ (1/t)^x with a variable exponent x between 1 and 1/2

At time t_127 the MIDI velocity shall be 127.

velocity(t) = 127 * (t_127/t)^x so that velocity(t_127) = 127
*/

//#define DEBUG_VELOCITY_TABLE

void initVelocities(float exponent) {
  float nominator = 127.0 * t_127;
  for (int t = 0; t < t_max; t++) {
    float velocity;
    if (t <= t_127)
      velocity = 127.0;
    else 
      velocity = 127.0 * pow((float)t_127 / t, exponent);
    if (velocity < velocity_min)
      velocity = velocity_min;
    velocities[t] = (byte)velocity;
    #ifdef DEBUG_VELOCITY_TABLE
    // print a bar chart
    for (int i = 0; i < velocities[t]; i++) {
      Serial.print('=');
    }
    Serial.println('|');
    #endif 
  }
}
