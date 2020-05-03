#pragma GCC optimize ("O3")
#include "Time.h"

// report measured key press times to serial?
//#define DEBUG_TIMES

// report maximum duration of 1 scanMatrix() call
//#define DEBUG_SCAN_TIME

/* 
keyboard scanned by diode matrix
*/

/**
 * Called if a state change (key pressed down or released) is detected by the matrix scanner.
 * @param key 0 = most left key, normally an A (88 keys) 
 * @param time measured time for key pressure in units of 128 microseconds, less than 0 for a released key
 */
void handleKeyEvent(byte key, int time);

const byte n_columns = 8; // neighbour keys belong to different columns
const byte n_rows = 11; // 8 * 11= 88 keys

/* 0 or when the key pressure started in Arduino milli seconds (uptime) */
long key_states[n_columns * n_rows] ; // all off, most left key = 0

/* normally closed pins (Ruhekontakte) */
const byte nc_row_pins[n_rows] = {
// Arduino pin // matrix pin ATMega port+bit
   43,         // R1         L6
   40,         // R2         G1
   53,         // R3         B0
   46,         // R4         L3
   52,         // R5         B1
   5,          // R6         E3
   37,         // R7         C0
   30,         // R8         C7
   31,         // R9         C6
   25,         // R10        A3
   24          // R11        A2
};

/* normally open pins (Arbeitskontakte) */
const byte no_row_pins[n_rows] = {
// Arduino pin // matrix pin ATMega port+bit
   47,         // A1         L2
   36,         // A2         C1
   49,         // A3         L0
   42,         // A4         L7
   48,         // A5         L1
   4,          // A6 marked  G5
   41,         // A7         G0
   34,         // A8         C3
   35,         // A9         C2
   29,         // A10        A7
   28          // A11        A6
};

const byte row_pins[n_rows] = {
// Arduino pin // 1..11      ATMega port+bit
   45,         // 1          L4
   38,         // 2          D7
   51,         // 3          B2
   44,         // 4          L5
   50,         // 5          B3 
   3,          // 6          E5
   39,         // 7          G2
   32,         // 8          C5
   33,         // 9          C4
   27,         // 10         A5
   26          // 11         A4
};

/* These are connected to the anodes of the diodes. */
const byte column_pins[n_columns] = {
// Arduino pin // matrix pin ATMega port+bit
   6,          // G1         H3        
   7,          // G2         H4
   8,          // G3         H5
   9,          // G4         H6
   10,         // G5         B4
   11,         // G6         B5
   12,         // G7         B6
   13          // G8 marked  B7
};

/**
 * To be called in Arduino setup().
 */
void setupMatrixPins() {
  for (byte i = 0; i < n_rows; i++) {
    pinMode(nc_row_pins[i], INPUT);
    pinMode(no_row_pins[i], INPUT);
    pinMode(row_pins[i], OUTPUT);
    digitalWrite(row_pins[i], HIGH);
  }
  for (byte i = 0; i < n_columns; i++) {
    pinMode(column_pins[i], OUTPUT);
    digitalWrite(column_pins[i], LOW);
  }
}

int max_scan_time_us;

// This procedure uses direct port manipulation and is around twice as fast as with digitalRead/Write.
// http://harperjiangnew.blogspot.de/2013/05/arduino-port-manipulation-on-mega-2560.html

/** 
 * To be called in loop.
 * Calls handleKeyEvent().
 */
void scanMatrix() {
  #ifdef DEBUG_SCAN_TIME
  unsigned long t_start = micros();
  #endif
  for (byte row = 0; row < n_rows; row++) {
    //digitalWrite(row_pins[row], LOW);
    switch (row) {
      case 0:  PORTL ^= (1<<4); break;
      case 1:  PORTD ^= (1<<7); break;
      case 2:  PORTB ^= (1<<2); break;
      case 3:  PORTL ^= (1<<5); break;
      case 4:  PORTB ^= (1<<3); break;
      case 5:  PORTE ^= (1<<5); break;
      case 6:  PORTG ^= (1<<2); break;
      case 7:  PORTC ^= (1<<5); break;
      case 8:  PORTC ^= (1<<4); break;
      case 9:  PORTA ^= (1<<5); break;
      case 10: PORTA ^= (1<<4); break;
    }
    for (byte column = 0; column < n_columns; column++) {
      //digitalWrite(column_pins[column], HIGH);
      switch (column) {
        case 0: PORTH |= (1<<3); break;
        case 1: PORTH |= (1<<4); break;
        case 2: PORTH |= (1<<5); break;
        case 3: PORTH |= (1<<6); break;
        case 4: PORTB |= (1<<4); break;
        case 5: PORTB |= (1<<5); break;
        case 6: PORTB |= (1<<6); break;
        case 7: PORTB |= (1<<7); break;
      }
      byte index = column + row * n_columns;
      long & state = key_states[index]; 
      // normally closed contact triggers start of time measurement and key-off
      //int value = digitalRead(nc_row_pins[row]);
      byte value;
      switch (row) {
        case 0:  value = PINL & (1<<6); break;
        case 1:  value = PING & (1<<1); break;
        case 2:  value = PINB & (1<<0); break;
        case 3:  value = PINL & (1<<3); break;
        case 4:  value = PINB & (1<<1); break;
        case 5:  value = PINE & (1<<3); break;
        case 6:  value = PINC & (1<<0); break;
        case 7:  value = PINC & (1<<7); break;
        case 8:  value = PINC & (1<<6); break;
        case 9:  value = PINA & (1<<3); break;
        case 10: value = PINA & (1<<2); break;
      }
      if (value != 0) {
        if (state < 0) {
          handleKeyEvent(index, -1);
        }
        state = 0;
      } else {
        if (state == 0) {
          // start of key-down
          //state = _128_micros(); // now state > 0
          state = micros() >> 7;
        }
      }
      // normally open contact triggers key-on
      //value = digitalRead(no_row_pins[row]);
      switch (row) {
        case 0:  value = PINL & (1<<2); break;
        case 1:  value = PINC & (1<<1); break;
        case 2:  value = PINL & (1<<0); break;
        case 3:  value = PINL & (1<<7); break;
        case 4:  value = PINL & (1<<1); break;
        case 5:  value = PING & (1<<5); break;
        case 6:  value = PING & (1<<0); break;
        case 7:  value = PINC & (1<<3); break;
        case 8:  value = PINC & (1<<2); break;
        case 9:  value = PINA & (1<<7); break;
        case 10: value = PINA & (1<<6); break;
      }
      if (value != 0 && state > 0) {
        //int t = _128_micros() - state;
        int t = (micros() >> 7) - state;
        handleKeyEvent(index, t);
        state = -1;
      }
      //digitalWrite(column_pins[column], LOW);
      switch (column) {
        case 0: PORTH ^= (1<<3); break;
        case 1: PORTH ^= (1<<4); break;
        case 2: PORTH ^= (1<<5); break;
        case 3: PORTH ^= (1<<6); break;
        case 4: PORTB ^= (1<<4); break;
        case 5: PORTB ^= (1<<5); break;
        case 6: PORTB ^= (1<<6); break;
        case 7: PORTB ^= (1<<7); break;
      }
    }
    //digitalWrite(row_pins[row], HIGH);    
    switch (row) {
      case 0:  PORTL |= (1<<4); break;
      case 1:  PORTD |= (1<<7); break;
      case 2:  PORTB |= (1<<2); break;
      case 3:  PORTL |= (1<<5); break;
      case 4:  PORTB |= (1<<3); break;
      case 5:  PORTE |= (1<<5); break;
      case 6:  PORTG |= (1<<2); break;
      case 7:  PORTC |= (1<<5); break;
      case 8:  PORTC |= (1<<4); break;
      case 9:  PORTA |= (1<<5); break;
      case 10: PORTA |= (1<<4); break;
    }
  }
  #ifdef DEBUG_SCAN_TIME
  unsigned long scan_time_us = micros() - t_start;
  if (scan_time_us > max_scan_time_us) {
    max_scan_time_us = scan_time_us;
    Serial.print(max_scan_time_us); Serial.println(" microseconds max. scan time");
    // measured max. scan time values: 
    // 2.0 ms direct port i/o, -Os gcc option (platform.txt)
    // 2.6 ms digitalRead/Write, -Os gcc option
    // 2.5 ms digitalRead/Write, -O2 gcc option
    // 2.0 ms digitalRead/Write, -O3 gcc option
    // 1.0 ms direct port i/o, -O3 gcc option
    // Seems that -Os does not work any more with direct i/o as used here since ?Arduino 1.8.11?.
  }
  #endif
}

 


