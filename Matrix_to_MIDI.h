#pragma GCC optimize ("O3")

// max. value of a MIDI controller
const byte MIDI_CONTROLLER_MAX = 127;
const byte MIDI_CONTROLLER_MEAN = 64;


const int meter_pin = 2;
const int meter_led_pin = A3;
const byte meter_max = 255; // = 1*3*5*17 -> possible deltas
const byte meter_delta = 1;
const byte global_delta = 17;
//const byte meter_mean = (meter_max / (meter_delta*2)) * meter_delta;
// a cheap Conrad Elektronik meter of course is very non-linear, better measure the value
// when the pointer is in the middle
const byte meter_mean = 155;

/**
 * Considers that the meter is mounted upside down in the case.
 * @value to be displayed
 */
void display(int value) {
  analogWrite(meter_pin, meter_max - value);
}

/**
 * Channel 1 -> pointer is on the left.
 * Channel 10 -> pointer is on the right.
 */
void displayChannel(midi::Channel ch) {
  display((ch - 1) * meter_max / 9);
}

// how much differences from the mean sensitivity are
// magnified 
const int zoom = 3;
/**
 * Magnifies the difference from the display mean value.
 * @value raw value
 * @return zoomed value
 */
byte magnify(byte value) {
  // value = meter_mean is not changed by the transformation
  int magnified = zoom * (value - meter_mean) + meter_mean;
  if (magnified > meter_max) return meter_max;
  if (magnified < 0) return 0;
  return magnified;
}

/*--------------------------------- persistent settings ---------------------------------*/

// the values in a fresh EEPROM
const byte invalid = 0xff;

/*--------- global settings ---------*/

float global_sens_to_exponent(byte sensitivity) {
  float ret = 1.0 - 0.5 * sensitivity / meter_max;
  Serial.print("exponent "); Serial.println(ret);
  return ret;
}

// start address of global settings storage area in EEPROM
const int SettingsAddress = 0;
const int n_keys = n_columns * n_rows;
typedef byte Sensitivities[n_keys]; 
Sensitivities sensitivities;

struct Settings {
  byte init;
  byte sensitivity;
  byte reserved2;
  byte reserved3;
  byte reserved4;
  byte reserved5;
  byte reserved6;
  byte reserved7;
  byte reserved8;
  byte reserved9;
  Sensitivities sensitivities;
};

Settings settings;

void printSettings() {
  Serial.print("global sensitivity "); Serial.println(settings.sensitivity);
  byte min_key_sens = 255; 
  byte max_key_sens = 0; 
  // find min. and max.
  for (int i = 0; i < n_keys; i++) {
    byte sens = settings.sensitivities[i];
    if (sens > max_key_sens)
      max_key_sens = sens;
    if (sens < min_key_sens)
      min_key_sens = sens;
  }
  // print
  for (int i = 0; i < n_keys; i++) {
    int sens = settings.sensitivities[i];
    Serial.print("sensitivity ");
    Serial.print(i);
    if (sens == max_key_sens) {
      Serial.print(" MAX ");
    } else if (sens == min_key_sens) {
      Serial.print(" min ");
    } else {
      Serial.print(" ");
    }
    Serial.println(sens);
  }
}

/**
 * Reads settings from EEPROM.
 * When the EEPROM still contains 0xff defaults,
 * reasonable initial values will be chosen for the settings.
 */
void readSettings() {
  // TODO checksum and detection of corrupt data
  byte *b = (byte*)&settings;
  for (int i = 0; i < sizeof(Settings); i++)
    b[i] = EEPROM.read(SettingsAddress + i);
  if (/*true ||*/ settings.init > 15) {
    // very first read 
    Serial.println("Starting with default settings!");
    Serial.print("All sensitivities will be "); Serial.println(meter_mean);
    settings.init = 0; 
    settings.sensitivity = meter_mean;
    for (int i = 0; i < n_keys; i++) {
      settings.sensitivities[i] = meter_mean;
    }
  }
  printSettings();
}

/**
 * Saves settings to EEPROM. To maximize EEPROM lifetime only changed values are actually written. 
 */
void saveSettings() {
  byte *b = (byte*)&settings;
  for (int i = 0; i < sizeof(Settings); i++) {
    byte original = EEPROM.read(SettingsAddress + i);
    if (original != b[i]) {
      EEPROM.write(SettingsAddress + i, b[i]);
      Serial.print("EEPROM ["); Serial.print(i); Serial.print("] <- "); Serial.println(b[i]);
    }
  }
}


/*--------------------------------- state event machine ---------------------------------*/

enum State { idle, global_sensitivity, key_sensitivity, wait_for_split, wait_for_preset};
enum Event { up_long, down_long, up_short, down_short, note_on, note_off, toggle_led, volume_knob, ext_ctrl };

void process(Event event, int value, int value2);
