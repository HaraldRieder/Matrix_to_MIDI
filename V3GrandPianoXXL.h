static midi::DataByte buff[10];

struct Sound {
  midi::DataByte bank;
  midi::DataByte prognum;
};

const struct Sound 
  GPViennaRock = { 0, 2 }, GPHamburgRock = { 0, 3 },
  GrandPianoVienna/*Bösendorfer*/ = { 0, 4 }, GrandPianoHamburg/*Steinway*/ = { 0, 5 }, 
  GPViennaLayeredPad = { 0, 16 }, GPHamburgLayeredPad = { 0, 17 }, 
  GPViennaLayeredStrings = { 0, 18 }, GPHamburgLayeredStrings = { 0, 19 },
  GPViennaDream = { 0, 20 }, GPHamburgDream = { 0, 21 },
  MK1DynoTremolo/*Fender Rhodes*/ = { 0, 31 }, MK1DynoLayeredPad = { 0, 32 }, MK1Tremolo = { 0, 37 }, MK1LayeredFM = { 0, 39 },
  A200 = { 0, 41 }, A200Tremolo1/*Wurlitzer*/ = { 0, 43 }, FMPianoLayeredMKS = { 0, 23 }, V3BellaLayeredCortales = { 0, 54 },
  Organ776555678fast = { 1, 1 }, Organ800000568fast = { 1, 3 }, Organ008530000fast = { 1, 5 }, Organ807800000slow = { 1, 8 },
  HammondFull = { 1, 20 },
  DigitalPad = { 2, 1 }, Brightness = { 2, 5 }, FairlySpace = { 2, 14 }, IceRain = { 2, 21 },  Ice = { 2, 45 }, 
  VPhrase = { 2, 47 }, M12Brass = { 2, 64 }, Brazza = { 2, 71 },
  Harpsichord = { 3, 47 }, HarpsichordOctave = { 3, 48 },
  GuitarNylon = { 3, 22 }, GuitarNylonSoft = { 3, 23 }, GuitarSteel = { 3, 26 }, GuitarSteelSoft = { 3, 27 },
  EGuitarClean = { 3, 43 }, EGuitarDistortion = { 3, 45 },
  HarpLong = { 3, 60 },
  UprJazzBassVel96 = { 3, 67 }, UprightJazzBass = { 3, 71 }, EBassUS1 = { 3, 79 }, EBassUS2 = { 3, 80 }, EBassPick = { 3, 82 }, EBassPickDark = { 3, 83 },
  EBassFretless = { 3, 84 }, SlapBass1 = { 3, 85 }, SlapBass2 = { 3, 86 },
  SynBass = { 2, 116 }, JBassSoft = { 3, 121 }, CSClassicBass = { 3, 122 }, MOBassENV = { 3, 124 },
  XBass1 = { 3, 125 }, XBass2 = { 3, 126 },
  Strings2Forte = { 4, 8 }, Strings3 = { 4, 13 }, 
  DiscoStringsLong = { 4, 16 }, StringsPWMA = { 4, 29 }, StringsM12D = { 4, 42 },
  ClassicChoirAahFilter = { 4, 65 },
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
  & EBassPickDark, & EBassFretless,
  & SlapBass1, & SlapBass2,
  & SynBass, & JBassSoft, 
  & CSClassicBass, & MOBassENV,
  & XBass1, & XBass2
};
const int n_bass_sounds = sizeof(bass_sounds) / sizeof (bass_sounds[0]);

/** 
 * corresponds to 1 line of a V3 Control registration
 */
struct Preset {
  const Sound & sound;
  midi::DataByte volume; // difference from 100
  midi::DataByte coarsetune; // difference from 0 [semitones]
  midi::DataByte reverb; // difference from 64
  midi::DataByte chorus; // abs. value
  midi::DataByte attack; // difference from 64
  midi::DataByte decay;  // difference from 64
  midi::DataByte release; // difference from 64
  midi::DataByte cutoff; // difference from 64
//  midi::DataByte finetune; // difference from 0 [cent]
//  midi::DataByte pan; // difference from center position
//  boolean mono; // 1 note only if true
};

/**
 * corresponds to a V3 Control registration
 */
struct Registration {
  const Preset left, right1, right2;
};

// TODO volumes, decays, releases all ok?
const struct Registration  
  AintNoSunshine = {  {UprightJazzBass,15,0,-50,0,0,0,10/*release*/}, {StringsPWMA}, {MK1Tremolo,-10} },
  AllCriedOut = { {SynBass,0,0,-50,0,0,0,10/*release*/}, {Ice}, {FairlySpace,-10} },
  Bedingungslos = { {EBassPickDark,15,0,-50,0,0,0,10/*release*/}, {VPhrase}, {GPViennaLayeredStrings,-10} },
  CatchTheRainbow = { {EBassUS2,15,0,-50,0,0,0,10/*release*/}, {ClassicChoirAahFilter}, {GPHamburgLayeredPad,-10} },
  DontPayTheFerryman = { {EBassUS2,15,0,-50,0,0,0,10/*release*/}, {Brazza}, {GPHamburgLayeredPad,-10} },
  DontYouNeed = { {EBassFretless,15,0,-30,0,0,0,18/*release*/}, {Brightness}, {GuitarSteelSoft,-41} },
  IchWillKeineSchokolade = { {UprJazzBassVel96,15,0,-50,0,0,0,10/*release*/}, {USTrumpetSection}, {Organ800000568fast,-55} }, 
  LetItRain = { {EBassUS2,15,0,-50,0,0,0,10/*release*/}, {HammondFull}, {MK1DynoTremolo,-10} },
  MeAndBobbyMcGee = { {EBassUS1,15,0,-50,0,0,0,10/*release*/}, {Organ776555678fast}, {Organ807800000slow,-41} }, 
  NieGenug = { {EBassUS1,15,0,-50}, {EGuitarDistortion,0,-12/*transpose*/,41,74,0,0,8/*release*/}, {EGuitarClean,-41,0,0,60/*chorus*/,0,0,17,-10/*cutoff*/} }, 
  RideLikeTheWind = { {MOBassENV,0,0,-50,0,0,0,4/*release*/}, {M12Brass}, {GPHamburgDream, -10} }, 
  RollingInTheDeep = { {EBassPickDark,15,0,-50,0,0,0,10/*release*/}, {Strings3,0,-12/*transpose*/}, {GPHamburgRock, -10} }, 
  SummerDreaming = { {EBassFretless,15,0,-50,0,0,10/*release*/}, {Organ008530000fast}, {A200,-10,0,0,16/*chorus*/} }, 
  ThatOleDevilCalledLove = { {UprightJazzBass,15,0,-50}, {USTrumpTrombSection}, {GrandPianoHamburg,-10} },
  ThisIsTheLife = { {EBassFretless,15,0,-50,0,0,18/*decay*/}, {Strings2Forte}, {GuitarSteel,-41,0,1,14/*chorus*/,0,0,6/*release*/} },
  ThisMasquerade = { {UprightJazzBass,15,0,-50,0,0,0,10/*release*/}, {Strings3}, {GrandPianoHamburg,-10} },
  UnderneathYourClothes = { {EBassFretless,15,0,-50,0,0,18/*release*/}, {DigitalPad,0,12,0,0,0,8/*release*/}, {StringsM12D,-41} };

const Registration * registrations[] = {
  & AintNoSunshine, & AllCriedOut,
  & Bedingungslos,
  & CatchTheRainbow,
  & DontPayTheFerryman, & DontYouNeed,
  & LetItRain,
  & IchWillKeineSchokolade,
  & MeAndBobbyMcGee,
  & NieGenug,
  & RideLikeTheWind, & RollingInTheDeep,
  & SummerDreaming,
  & ThatOleDevilCalledLove, & ThisIsTheLife, & ThisMasquerade,
  & UnderneathYourClothes
};

const int n_registrations = sizeof(registrations) / sizeof (registrations[0]);

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

void sendSound(const Sound * sound, 
               midi::Channel channel, 
               midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  interface.sendControlChange(midi::BankSelect, sound->bank, channel); // only MSB necessary for V3 Sound Grand Piano XXL
  interface.sendProgramChange(sound->prognum, channel);
  interface.sendControlChange(0x77, 0, channel); // reset all NRPNs
};

void sendPreset(const Preset * preset, 
                midi::Channel channel, 
                midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  sendSound(&preset->sound, channel, interface);
  interface.sendControlChange(0x77, 0, channel); // reset all NRPNs
  interface.sendControlChange(midi::ChannelVolume, MIDI_CONTROLLER_MAX & (preset->volume + 100), channel);
  interface.sendControlChange(midi::ExpressionController, MIDI_CONTROLLER_MAX, channel); // reset expression value
  sendCoarseTune(MIDI_CONTROLLER_MAX & (preset->coarsetune + 64), channel, interface);
  interface.sendControlChange(midi::Effects1, MIDI_CONTROLLER_MAX & (preset->reverb + 64), channel);
  interface.sendControlChange(midi::Effects3, MIDI_CONTROLLER_MAX & preset->chorus, channel);
  interface.sendControlChange(midi::SoundController4, MIDI_CONTROLLER_MAX & (preset->attack + 64), channel);
  interface.sendControlChange(midi::SoundController6, MIDI_CONTROLLER_MAX & (preset->decay + 64), channel);
  interface.sendControlChange(midi::SoundController3, MIDI_CONTROLLER_MAX & (preset->release + 64), channel);
  interface.sendControlChange(midi::SoundController5, MIDI_CONTROLLER_MAX & (preset->cutoff + 64), channel);
  // TODO send the rest
};

void sendRegistration(const Registration * registration, 
                 midi::Channel base_channel,
                 midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
  sendPreset(&registration->left  , base_channel    , interface);                 
  sendPreset(&registration->right1, base_channel + 1, interface);                 
  sendPreset(&registration->right2, base_channel + 2, interface);
  sendFineTune(0x38, base_channel    , interface);
  sendFineTune(0x40, base_channel + 1, interface);
  sendFineTune(0x42, base_channel + 2, interface);
};
