struct Sound {
  midi::DataByte bank;
  midi::DataByte prognum;
};

const struct Sound 
  GrandPianoVienna/*Bösendorfer*/ = { 0, 4 }, GrandPianoHamburg/*Steinway*/ = { 0, 5 }, 
  GPViennaLayeredPad = { 0, 16 }, GPHamburgLayeredPad = { 0, 17 }, 
  GPViennaLayeredStrings = { 0, 18 }, GPHamburgLayeredStrings = { 0, 19 },
  MK1DynoTremolo/*Fender Rhodes*/ = { 0, 31 }, MK1DynoLayeredPad = { 0, 32 }, MK1Tremolo = { 0, 37 }, MK1LayeredFM = { 0, 39 },
  A200Tremolo1/*Wurlitzer*/ = { 0, 41 }, FMPianoLayeredMKS = { 0, 51 }, V3BellaLayeredCortales = { 0, 54 },
  Harpsichord = { 3, 47 }, HarpsichordOctave = { 3, 48 },
  GuitarNylon = { 3, 22 }, GuitarNylonSoft = { 3, 23 }, GuitarSteel = { 3, 26 }, GuitarSteelSoft = { 3, 27 },
  HarpLong = { 3, 60 },
  UprightJazzBass = { 3, 71 }, EBassUS1 = { 3, 79 }, EBassUS2 = { 3, 80 }, EBassPick = { 3, 82 }, EBassPickDark = { 3, 83 },
  EBassFretless = { 3, 84 }, SlapBass1 = { 3, 85 }, SlapBass2 = { 3, 86 },
  SynBass = { 2, 116 }, JBassSoft = { 3, 121 }, CSClassicBass = { 3, 122 }, MOBassENV = { 3, 124 },
  XBass1 = { 3, 125 }, XBass2 = { 3, 126 },
  PopDrumKit = { 4, 120 }, JazzDrumKit = { 4, 121 }, OrchestraPercussion = { 4, 122 }, VoiceKit = { 4, 123 },
  NoSound = { 0, 127 };

// my favourite non-split and accompanying sounds
const Sound * sounds[] = {
  & GrandPianoHamburg, & GrandPianoVienna, 
  & GPViennaLayeredPad, & GPHamburgLayeredPad,
  & GPHamburgLayeredStrings, & GPViennaLayeredStrings,
  & Harpsichord, & HarpsichordOctave,
  & MK1DynoTremolo, & MK1DynoLayeredPad,
  & MK1Tremolo, & MK1LayeredFM,
  & A200Tremolo1, & FMPianoLayeredMKS,
  & V3BellaLayeredCortales, & NoSound, // reserved
  & GuitarNylon, & GuitarNylonSoft,
  & GuitarSteel, & GuitarSteelSoft,
  & HarpLong, & NoSound, // reserved
  & PopDrumKit, & JazzDrumKit,
  & OrchestraPercussion, & VoiceKit
};
const int n_sounds = sizeof(sounds) / sizeof (sounds[0]);

const Sound * bass_sounds[] = {
  & UprightJazzBass, & EBassUS1, 
  & EBassUS2, & EBassPick, 
  & EBassPickDark,
  & EBassFretless,
  & SlapBass1, & SlapBass2,
  & SynBass, & JBassSoft, 
  & CSClassicBass, & MOBassENV,
  & XBass1, & XBass2
};
const int n_bass_sounds = sizeof(bass_sounds) / sizeof (bass_sounds[0]);

void sendSound(const Sound * sound, 
               midi::Channel channel, 
               midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
    interface.sendControlChange(midi::BankSelect, sound->bank, channel); // only MSB necessary for V3 Sound Grand Piano XXL
    interface.sendProgramChange(sound->prognum, channel);
    // reset all NRPNs
    interface.sendControlChange(0x77, 0, channel);
};

struct RightLayer {
  const Sound * right1, * right2;
};

const struct RightLayer 
  Jazz = { & GrandPianoHamburg, & NoSound },
  Soul = { & MK1DynoTremolo, & NoSound };

const RightLayer * right_layers[] = {
  & Jazz,
  & Soul
};

const int n_right_layers = sizeof(right_layers) / sizeof (right_layers[0]);
