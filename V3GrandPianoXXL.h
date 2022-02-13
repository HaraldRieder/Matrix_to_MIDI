struct Sound {
  midi::DataByte bank;
  midi::DataByte prognum;
  midi::DataByte volume;
};

const struct Sound 
  GrandPianoVienna/*Bösendorfer*/ = { 0, 4, 100 }, GrandPianoHamburg/*Steinway*/ = { 0, 5, 100 }, 
  GPViennaLayeredPad = { 0, 16, 100 }, GPHamburgLayeredPad = { 0, 17, 100 }, 
  GPViennaLayeredStrings = { 0, 18, 100 }, GPHamburgLayeredStrings = { 0, 19, 100 },
  MK1DynoTremolo/*Fender Rhodes*/ = { 0, 31, 100 }, MK1DynoLayeredPad = { 0, 32, 100 }, MK1Tremolo = { 0, 37, 100 }, MK1LayeredFM = { 0, 39, 100 },
  A200Tremolo1/*Wurlitzer*/ = { 0, 41, 100 }, FMPianoLayeredMKS = { 0, 5, 1001 }, V3BellaLayeredCortales = { 0, 54, 100 },
  Harpsichord = { 3, 47, 100 }, HarpsichordOctave = { 3, 48, 100 },
  GuitarNylon = { 3, 22, 100 }, GuitarNylonSoft = { 3, 23, 100 }, GuitarSteel = { 3, 26, 100 }, GuitarSteelSoft = { 3, 27, 100 },
  HarpLong = { 3, 60, 100 },
  UprightJazzBass = { 3, 71, 100 }, EBassUS1 = { 3, 79, 100 }, EBassUS2 = { 3, 80, 100 }, EBassPick = { 3, 82, 100 }, EBassPickDark = { 3, 83, 100 },
  EBassFretless = { 3, 84, 100 }, SlapBass1 = { 3, 85, 100 }, SlapBass2 = { 3, 86, 100 },
  SynBass = { 2, 116, 100 }, JBassSoft = { 3, 121, 100 }, CSClassicBass = { 3, 122, 100 }, MOBassENV = { 3, 124, 100 },
  XBass1 = { 3, 125, 100 }, XBass2 = { 3, 126, 100 },
  DiscoStringsLong = { 4, 16, 100 },
  USTrumpTrombSection = { 5, 32, 100 },
  PopDrumKit = { 4, 120, 100 }, JazzDrumKit = { 4, 121, 100 }, OrchestraPercussion = { 4, 122, 100 }, VoiceKit = { 4, 123, 100 },
  NoSound = { 0, 127, 100 };

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

void sendSoundWithVolume(const Sound * sound, int master_volume,
                         midi::Channel channel, 
                         midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  sendSound(sound, channel, interface);
  interface.sendControlChange(midi::ChannelVolume, sound->volume*master_volume/MIDI_CONTROLLER_MAX, channel);
};

             
struct RightLayer {
  const Sound * right1, * right2;
};

const struct RightLayer 
  Jazz = { & GrandPianoHamburg, & USTrumpTrombSection },
  Soul = { & MK1DynoTremolo, & DiscoStringsLong };

const RightLayer * right_layers[] = {
  & Jazz,
  & Soul
};

const int n_right_layers = sizeof(right_layers) / sizeof (right_layers[0]);
