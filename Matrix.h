/* 
keyboard scanned by diode matrix
*/

/**
 * Called if a state change is detected by the matrix scanner.
 * @param key 0 = most left key
 * @param on true if the key has been pressed, else it has been released
 */
void handleKeyEvent(int key, boolean on);

const int n_columns = 8; // neighbours belong to different columns
const int n_rows = 2; // cable supports up to 3, but 2 are sufficient for 15 pedals

boolean key_states[n_columns * n_rows] ; // all off, most left pedal = 0

/* normally closed pins (Ruhekontakte) */
const int nc_row_pins[n_rows] = {
  30,  // R12 green-brown
  34  // R13 green
//  38  // R14 green-white
};

/* normally open pins (Arbeitskontakte) */
const int no_row_pins[n_rows] = {
  28,  // A12 red-black
  32  // A13 red
//  36  // R14 rose
};

const int row_pins[n_rows] = {
  40, // 
  42
//  44
};

/* These are connected to the anodes of the diodes. */
const int column_pins[n_columns] = {
  29,  // G1 black
  31,  // G2
  33,  // G3
  35,  // G4
  37,  // G5
  39,  // G6
  41,  // G7
  43  // G8 violet
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
      boolean & state = pedal_states[index]; 
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

