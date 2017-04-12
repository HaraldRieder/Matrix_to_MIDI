// max. value of a MIDI controller
const byte MIDI_CONTROLLER_MAX = 127;
const byte MIDI_CONTROLLER_MEAN = 64;


/*--------------------------------- persistent settings ---------------------------------*/

// the values in a fresh EEPROM
const byte invalid = 0xff;

/*--------- global settings ---------*/
// start address of global settings storage area in EEPROM
const int GlobalSettingsAddress = 0;

struct GlobalSettings {
  byte output_channel;
  byte reserved2;
  byte reserved3;
  byte reserved4;
  byte reserved5;
  byte reserved6;
  byte reserved7;
  byte reserved8;
  byte reserved9;
};

GlobalSettings globalSettings;

/**
 * Reads global settings from EEPROM.
 * When the EEPROM still contains 0xff defaults,
 * reasonable initial values will be chosen for the global settings.
 */
void readGlobals() {
  // TODO checksum and detection of corrupt data
  byte *b = (byte*)&globalSettings;
  for (int i = 0; i < sizeof(GlobalSettings); i++)
    b[i] = EEPROM.read(GlobalSettingsAddress + i);
  if (globalSettings.output_channel > 15)
    globalSettings.output_channel = 0; // 1st MIDI channel
}

/**
 * Saves global settings to EEPROM. To maximize EEPROM lifetime only changed values are actually written. 
 */
void saveGlobals() {
  byte *b = (byte*)&globalSettings;
  for (int i = 0; i < sizeof(GlobalSettings); i++) {
    byte original = EEPROM.read(GlobalSettingsAddress + i);
    if (original != b[i])
      EEPROM.write(GlobalSettingsAddress + i, b[i]);
  }
}

const int n_keys = 88;
typedef byte Sensitivities[n_keys]; 

Sensitivities sensitivities;

// start address of key velocity storage area in EEPROM
const int SensivitiesAddress = 40;

void readSensitivities() {
  // TODO checksum and detection of corrupt data
  byte *b = (byte*)&sensitivities;
  for (int i = 0; i < sizeof(Sensitivities); i++)
    b[i] = EEPROM.read(SensivitiesAddress + i);
}

void saveSensitivities() {
  byte *b = (byte*)&sensitivities;
  for (int i = 0; i < sizeof(Sensitivities); i++) {
    byte original = EEPROM.read(SensivitiesAddress + i);
    if (original != b[i])
      EEPROM.write(SensivitiesAddress + i, b[i]);
  }
}


/*--------------------------------- state event machine ---------------------------------*/

enum State {
};

enum Event {
};

void process(Event event, int value);

