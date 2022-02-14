static midi::DataByte buff[10];

/**
 * SYSEX F0H 7FH 7FH 04H 01H 00H II F7H 
 * Master volume (II=0 to 127, default 127)
 */
void sendMasterVolume(midi::DataByte volume,
                      midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  buff[0] = 0x7f;
  buff[1] = 0x7f;
  buff[2] = 0x04;
  buff[3] = 0x01;
  buff[4] = 0x00;
  buff[5] = volume;
  interface.sendSysEx(6, buff);
};

/**
 * Fine tune in cent BnH 65H 00H 64H 01H 06H vv
 * vv=00 -100 vv=40H 0 vv=7FH +100
 */
void sendFineTune(midi::DataByte value, 
                  midi::Channel channel, 
                  midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  buff[0] = 0xB0 + channel;
  buff[1] = 0x65;
  buff[2] = 0x00;
  buff[3] = 0x64;
  buff[4] = 0x01;
  buff[5] = 0x06;
  buff[6] = value;
  interface.sendSysEx(7, buff, true);                    
}

/**
 * Fine tune in cent BnH 65H 00H 64H 01H 06H vv
 * vv=00 -100 vv=40H 0 vv=7FH +100
 */
void sendCoarseTune(midi::DataByte value, 
                   midi::Channel channel, 
                   midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  buff[0] = 0xB0 + channel;
  buff[1] = 0x65;
  buff[2] = 0x00;
  buff[3] = 0x64;
  buff[4] = 0x02;
  buff[5] = 0x06;
  buff[6] = value;
  interface.sendSysEx(7, buff, true);                    
}

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
  DiscoStringsLong = { 4, 16, 100 }, StringsPWMA = { 4, 29, 100 },
  USTrumpetSection = { 5, 32, 100 }, USTrumpTrombSection = { 5, 42, 100 },
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

void sendVolume(midi::DataByte volume, 
                midi::Channel channel, 
                midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  interface.sendControlChange(midi::ChannelVolume, volume, channel);
};

void sendSound(const Sound * sound, 
               midi::Channel channel, 
               midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  interface.sendControlChange(midi::BankSelect, sound->bank, channel); // only MSB necessary for V3 Sound Grand Piano XXL
  interface.sendProgramChange(sound->prognum, channel);
  interface.sendControlChange(0x77, 0, channel); // reset all NRPNs
  sendVolume(sound->volume, channel, interface);
};

struct RightLayer {
  const Sound * right1, * right2;
  midi::DataByte coarseTuneRight1, coarseTuneRight2;
};

const struct RightLayer 
  Jazz = { & GrandPianoHamburg, & USTrumpTrombSection, 0x40, 0x40 },
  Ferryman = { & GPViennaLayeredPad, & USTrumpetSection, 0x40, 0x40 },
  Soul = { & MK1Tremolo, & StringsPWMA, 0x40, 0x4C };

const RightLayer * right_layers[] = {
  & Jazz, & Ferryman, & Soul
};

const int n_right_layers = sizeof(right_layers) / sizeof (right_layers[0]);
