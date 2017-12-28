// max. value of a MIDI controller
const byte MIDI_CONTROLLER_MAX = 127;
const byte MIDI_CONTROLLER_MEAN = 64;


/*--------------------------------- persistent settings ---------------------------------*/

const byte meter_max = 255; // = 3*5*17 -> possible deltas
const byte meter_delta = 5;
const byte meter_mean = (meter_max / (meter_delta*2)) * meter_delta;


// the values in a fresh EEPROM
const byte invalid = 0xff;

/*--------- global settings ---------*/
// start address of global settings storage area in EEPROM
const int SettingsAddress = 0;
const int n_keys = n_columns * n_rows;
typedef byte Sensitivities[n_keys]; 
Sensitivities sensitivities;

struct Settings {
  byte output_channel;
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
  if (/*true ||*/ settings.output_channel > 15) {
    // very first read 
    Serial.println("Starting with default settings!");
    Serial.print("All sensitivities will be "); Serial.println(meter_mean);
    settings.output_channel = 0; // 1st MIDI channel
    settings.sensitivity = meter_mean;
    for (int i = 0; i < n_keys; i++) {
      settings.sensitivities[i] = meter_mean;
    }
  }
  Serial.print("Output channel "); Serial.println(settings.output_channel);
  Serial.print("Global sensitivity "); Serial.println(settings.sensitivity);
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

enum State { idle, global_sensitivity, key_sensitivity};
enum Event { up_long, down_long, up_short, down_short, note_on, note_off };

void process(Event event, int value);

