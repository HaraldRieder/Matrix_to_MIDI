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
 * Coarse tune in half tones BnH 65H 00H 64H 02H 06H vv
 * vv=00 -64 vv=40H 0 vv=7FH +64
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

// corresponds to 1 line of a V3 control registration
struct Sound {
  midi::DataByte bank;
  midi::DataByte prognum;
  midi::DataByte volume; // difference from 100
//  midi::DataByte pan; // difference from center position
  midi::DataByte coarsetune; // difference from 0 [semitones]
//  midi::DataByte finetune; // difference from 0 [cent]
  midi::DataByte reverb; // difference from 64
  midi::DataByte chorus; // difference from 64
  midi::DataByte attack; // difference from 64
  midi::DataByte decay;  // difference from 64
  midi::DataByte release; // difference from 64
  midi::DataByte cutoff; // difference from 64
//  boolean mono; // 1 note only if true
};

const struct Sound 
  GrandPianoVienna/*Bösendorfer*/ = { 0, 4 }, GrandPianoHamburg/*Steinway*/ = { 0, 5 }, 
  GPViennaLayeredPad = { 0, 16 }, GPHamburgLayeredPad = { 0, 17 }, 
  GPViennaLayeredStrings = { 0, 18 }, GPHamburgLayeredStrings = { 0, 19 },
  MK1DynoTremolo/*Fender Rhodes*/ = { 0, 31 }, MK1DynoLayeredPad = { 0, 32 }, MK1Tremolo = { 0, 37 }, MK1LayeredFM = { 0, 39 },
  A200Tremolo1/*Wurlitzer*/ = { 0, 41 }, FMPianoLayeredMKS = { 0, 5 }, V3BellaLayeredCortales = { 0, 54 },
  Harpsichord = { 3, 47 }, HarpsichordOctave = { 3, 48 },
  GuitarNylon = { 3, 22 }, GuitarNylonSoft = { 3, 23 }, GuitarSteel = { 3, 26 }, GuitarSteelSoft = { 3, 27 },
  HarpLong = { 3, 60 },
  UprightJazzBass = { 3, 71 }, EBassUS1 = { 3, 79 }, EBassUS2 = { 3, 80 }, EBassPick = { 3, 82 }, EBassPickDark = { 3, 83 },
  EBassFretless = { 3, 84 }, SlapBass1 = { 3, 85 }, SlapBass2 = { 3, 86 },
  SynBass = { 2, 116 }, JBassSoft = { 3, 121 }, CSClassicBass = { 3, 122 }, MOBassENV = { 3, 124 },
  XBass1 = { 3, 125 }, XBass2 = { 3, 126 },
  DiscoStringsLong = { 4, 16 }, StringsPWMA = { 4, 29 },
  USTrumpetSection = { 5, 32 }, USTrumpTrombSection = { 5, 42 },
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
  sendVolume(MIDI_CONTROLLER_MAX & (sound->volume + 100), channel, interface);
  sendCoarseTune(MIDI_CONTROLLER_MAX & (sound->coarsetune + 100), channel + 1, interface);
  interface.sendControlChange(midi::Effects1, MIDI_CONTROLLER_MAX & (sound->reverb + 64), channel);
  interface.sendControlChange(midi::Effects3, MIDI_CONTROLLER_MAX & (sound->chorus + 64), channel);
  interface.sendControlChange(midi::SoundController4, MIDI_CONTROLLER_MAX & (sound->attack + 64), channel);
  interface.sendControlChange(midi::SoundController6, MIDI_CONTROLLER_MAX & (sound->decay + 64), channel);
  interface.sendControlChange(midi::SoundController3, MIDI_CONTROLLER_MAX & (sound->release + 64), channel);
  interface.sendControlChange(midi::SoundController5, MIDI_CONTROLLER_MAX & (sound->cutoff + 64), channel);
  // TODO send the rest
};

struct Registration {
  const Sound left, right1, right2;
};

const struct Registration  
  ThatOleDevilCalledLove = Registration { Sound{UprightJazzBass}, Sound{GrandPianoHamburg}, Sound{USTrumpTrombSection} };
//  DontPayTheFerryman = { & GPViennaLayeredPad, & USTrumpetSection, 0x40, 0x40 },
//  Soul = { & MK1Tremolo, & StringsPWMA, 0x40, 0x4C };

const Registration * registrations[] = {
  & ThatOleDevilCalledLove//, & DontPayTheFerryman, & Soul
};

const int n_registrations = sizeof(registrations) / sizeof (registrations[0]);

void sendRegistration(const Registration * registration, 
                 midi::Channel base_channel,
                 midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  sendSound(&registration->left  , base_channel    , interface);                 
  sendSound(&registration->right1, base_channel + 1, interface);                 
  sendSound(&registration->right2, base_channel + 2, interface);
  sendFineTune(0x38, base_channel    , interface);
  sendFineTune(0x40, base_channel + 1, interface);
  sendFineTune(0x42, base_channel + 2, interface);
};
