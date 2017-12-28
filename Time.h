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

