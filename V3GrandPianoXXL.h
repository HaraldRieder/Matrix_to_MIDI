struct PresetSound {
  midi::DataByte bank;
  midi::DataByte prognum;
};

const struct PresetSound 
  GrandPianoVienna = { 0, 4 }, // Bösendorfer
  GrandPianoHamburg = { 0, 5 }, // Steinway
  GPViennaLayeredPad = { 0, 16 },
  GPHamburgLayeredPad = { 0, 17 },
  GPViennaLayeredStrings = { 0, 18 },
  GPHamburgLayeredStrings = { 0, 19 },
  MK1DynoTremolo = { 0, 31 }, // Fender Rhodes
  MK1DynoLayeredPad = { 0, 32 },
  MK1Tremolo = { 0, 37 },
  MK1LayeredFM = { 0, 39 },
  A200Tremolo1 = { 0, 41 }, // Wurlitzer
  FMPianoLayeredMKS = { 0, 51 },
  V3BellaLayeredCortales = { 0, 54 },
  Harpsichord = { 3, 47 },
  HarpsichordOctave = { 3, 48 },
  GuitarNylon = { 3, 22 },
  GuitarNylonSoft = { 3, 23 },
  GuitarSteel = { 3, 26 },
  GuitarSteelSoft = { 3, 27 },
  HarpLong = { 3, 60 },
  UprightJazzBass = { 3, 71 }, 
  EBassUS1 = { 3, 79 },
  EBassUS1 = { 3, 80 },
  EBassPick = { 3, 82 },
  EBassPickDark = { 3, 83 },
  EBassFretless = { 3, 84 },
  SlapBass1 = { 3, 85 },
  SlapBass2 = { 3, 86 },
  SynBass = { 2, 116 },
  JBassSoft = { 3, 121 },
  CSClassicBass = { 3, 122 },
  MOBassENV = { 3, 124 },
  XBass1 = { 3, 125 },
  XBass2 = { 3, 126 },
  PopDrumKit = { 4, 120 },
  JazzDrumKit = { 4, 121 },
  OrchestraPercussion = { 4, 122 },
  VoiceKit = { 4, 123 },
  NoSound = { 0, 127 };

// my favourite non-split sounds
const PresetSound preset_sounds[] = {
  GrandPianoHamburg, GrandPianoVienna, 
  GPViennaLayeredPad, GPHamburgLayeredPad,
  GPHamburgLayeredStrings, GPViennaLayeredStrings,
  Harpsichord, HarpsichordOctave,
  MK1DynoTremolo, MK1DynoLayeredPad,
  MK1Tremolo, MK1LayeredFM,
  A200Tremolo1, FMPianoLayeredMKS,
  V3BellaLayeredCortales, NoSound, // reserved
  GuitarNylon, GuitarNylonSoft,
  GuitarSteel, GuitarSteelSoft,
  HarpLong, NoSound, // reserved
  PopDrumKit = { 4, 120 },
  JazzDrumKit = { 4, 121 },
  OrchestraPercussion = { 4, 122 },
  VoiceKit = { 4, 123 }
};

const int n_preset_sounds = sizeof(preset_sounds) / sizeof (preset_sounds[0]);

void sendSound(const PresetSound & sound, 
               midi::Channel channel, 
               midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
    interface.sendControlChange(midi::BankSelect, sound.bank, channel); // only MSB necessary for V3 Sound Grand Piano XXL
    interface.sendProgramChange(sound.prognum, channel);
    // reset all NRPNs
    interface.sendControlChange(0x77, 0, channel);
};

struct PresetSplit {
  const PresetSound left, right1, right2;
};

const struct PresetSplit 
  Jazz = { JazzBass, GrandPianoHamburg, NoSound },
  Soul = { BassUS1, RhodesMK1Soft, NoSound };

const PresetSplit preset_splits[] = {
  Jazz,
  Soul
};

const int n_preset_splits = sizeof(preset_splits) / sizeof (preset_splits[0]);
