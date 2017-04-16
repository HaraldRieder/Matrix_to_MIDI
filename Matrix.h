/* 
keyboard scanned by diode matrix
*/

/**
 * Called if a state change is detected by the matrix scanner.
 * @param key 0 = most left key, normally an A (88 keys) 
 * @param on true if the key has been pressed, else it has been released
 */
void handleKeyEvent(int key, boolean on);

const int n_columns = 8; // neighbour keys belong to different columns
const int n_rows = 11; // 8 * 11= 88 keys

boolean key_states[n_columns * n_rows] ; // all off, most left key = 0

/* normally closed pins (Ruhekontakte) */
const int nc_row_pins[n_rows] = {
  43, // R1 
  40, // R2 
  53, // R3 
  46, // R4
  52, // R5 
  5,  // R6 
  37, // R7 
  30, // R8 
  31, // R9 
  25, // R10 
  24  // R11 
};

/* normally open pins (Arbeitskontakte) */
const int no_row_pins[n_rows] = {
  47, // A1 
  36, // A2 
  49, // A3 
  42, // A4 
  48, // A5 
  4,  // A6 marked
  41, // A7 
  34, // A8 
  35, // A9 
  29, // A10 
  28  // A11 
};

const int row_pins[n_rows] = {
  45, // 1 
  38, // 2 
  51, // 3 
  44, // 4 
  50, // 5 
  3,  // 6 
  39, // 7 
  32, // 8 
  33, // 9 
  27, // 10
  26  // 11
};

/* These are connected to the anodes of the diodes. */
const int column_pins[n_columns] = {
  6,  // G1
  7,  // G2
  8,  // G3
  9,  // G4
  10, // G5
  11, // G6
  12, // G7
  13  // G8 marked
};

/**
 * To be called in Arduino setup().
 */
void setupMatrixPins() {
  for (int i = 0; i < n_rows; i++) {
    pinMode(nc_row_pins[i], INPUT);
    pinMode(no_row_pins[i], INPUT);
    pinMode(row_pins[i], OUTPUT);
    digitalWrite(row_pins[i], HIGH);
  }
  for (int i = 0; i < n_columns; i++) {
    pinMode(column_pins[i], OUTPUT);
    digitalWrite(column_pins[i], LOW);
  }
}

int max_scan_time_ms;

/**
 * To be called in loop.
 * Calls handleKeyEvent().
 */
void scanMatrix() {
  int t_start = millis();
  for (int row = 0; row < n_rows; row++) {
    digitalWrite(row_pins[row], LOW);
    for (int column = 0; column < n_columns; column++) {
      digitalWrite(column_pins[column], HIGH);
      int index = column + row * n_columns;
      boolean & state = key_states[index]; 
      int value = digitalRead(nc_row_pins[row]);
      if (value == HIGH && state) {
        handleKeyEvent(index, state = false);
      }
      value = digitalRead(no_row_pins[row]);
      if (value == HIGH && !state) {
        handleKeyEvent(index, state = true);
      }
      digitalWrite(column_pins[column], LOW);
    }
    digitalWrite(row_pins[row], HIGH);    
  }
  max_scan_time_ms = max(max_scan_time_ms, millis() - t_start);
}

