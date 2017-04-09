// max. value of a MIDI controller
const byte MIDI_CONTROLLER_MAX = 127;
const byte MIDI_CONTROLLER_MEAN = 64;


/*--------------------------------- persistent settings ---------------------------------*/

// the values in a fresh EEPROM
const byte invalid = 0xff;

/*--------- global settings ---------*/
// start address of global settings storage area in EEPROM
const int GlobalSettingsAddress = 0;

const int n_global_settings = 6;

enum GlobalParameter {
  BassBoostParam, BoostFreqParam, 
  VelocitySlopeParam, VelocityOffsetParam, 
  FilterVelocitySlopeParam, FilterVelocityOffsetParam
};

struct GlobalSettings {
  byte SD2_bass_boost;
  byte SD2_boost_freq;
  byte SD2_velo_slope;
  byte SD2_velo_offset;
  byte SD2_filter_velo_slope;
  byte SD2_filter_velo_offset;
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
  // todo
  // TODO checksum and detection of corrupt data
  byte *b = (byte*)&globalSettings;
  for (int i = 0; i < sizeof(GlobalSettings); i++)
    b[i] = EEPROM.read(GlobalSettingsAddress + i);
  if (globalSettings.SD2_bass_boost > max_boost_gain)
    globalSettings.SD2_bass_boost = 0;
  if (globalSettings.SD2_boost_freq > max_boost_freq)
    globalSettings.SD2_boost_freq = max_boost_freq/2;
  if (globalSettings.SD2_velo_slope > MIDI_CONTROLLER_MAX)
    globalSettings.SD2_velo_slope = MIDI_CONTROLLER_MEAN;
  if (globalSettings.SD2_velo_offset > MIDI_CONTROLLER_MAX)
    globalSettings.SD2_velo_offset = MIDI_CONTROLLER_MEAN;
  if (globalSettings.SD2_filter_velo_slope > MIDI_CONTROLLER_MAX)
    globalSettings.SD2_filter_velo_slope = MIDI_CONTROLLER_MEAN;
  if (globalSettings.SD2_filter_velo_offset > MIDI_CONTROLLER_MAX)
    globalSettings.SD2_filter_velo_offset = MIDI_CONTROLLER_MEAN;
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

// start address of key velocity storage area in EEPROM
const int KeyVelocityAddress = 40;

/*--------------------------------- state event machine ---------------------------------*/

enum State {
};

enum Event {
};

void process(Event event, int value);








