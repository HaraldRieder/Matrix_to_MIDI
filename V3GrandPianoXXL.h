struct PresetSound {
  midi::DataByte bank;
  midi::DataByte prognum;
};

const struct PresetSound 
  GrandViennaSoft = { 0, 6 },
  GrandPianoHamburg = { 0, 1 },
  RhodesMK1Soft = { 0, 36 };

const PresetSound *preset_sounds[] = {
  & GrandViennaSoft,
  & GrandPianoHamburg,
  & RhodesMK1Soft
};

const int n_preset_sounds = sizeof(preset_sounds) / sizeof (preset_sounds[0]);

void sendSound(const PresetSound *sound, 
               midi::Channel channel, 
               midi::MidiInterface<midi::SerialMIDI<HardwareSerial>> & interface) {
    interface.sendControlChange(midi::BankSelect, sound->bank, channel); // only MSB necessary for V3 Sound Grand Piano XXL
    interface.sendProgramChange(sound->prognum, channel);
    // reset all NRPNs
    interface.sendControlChange(0x77, 0, channel);
}
