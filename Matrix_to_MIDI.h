// max. value of a MIDI controller
const byte MIDI_CONTROLLER_MAX = 127;
const byte MIDI_CONTROLLER_MEAN = 64;


/*--------------------------------- persistent settings ---------------------------------*/

const byte meter_max = 255; // = 3*5*17 = possible deltas
const byte meter_delta = 17;


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
  if (settings.output_channel > 15) {
    // very first read 
    settings.output_channel = 0; // 1st MIDI channel
    int mean_value = (meter_max / (meter_delta*2)) * meter_delta;
    settings.sensitivity = mean_value;
    for (int i = 0; i < n_keys; i++) {
      settings.sensitivities[i] = mean_value;
    }
  }
}

/**
 * Saves settings to EEPROM. To maximize EEPROM lifetime only changed values are actually written. 
 */
void saveSettings() {
  byte *b = (byte*)&settings;
  for (int i = 0; i < sizeof(Settings); i++) {
    byte original = EEPROM.read(SettingsAddress + i);
    if (original != b[i])
      EEPROM.write(SettingsAddress + i, b[i]);
  }
}


/*--------------------------------- state event machine ---------------------------------*/

enum State { idle, global_sensitivity, key_sensitivity};
enum Event { up_long, down_long, up_short, down_short, note_on };

void process(Event event, int value);

