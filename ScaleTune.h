/* Roland GS Scale Tune Messages */

const int N_KEYS = 12;
typedef int KeyArray[N_KEYS];

/* just intonation with clean fifths and thirds
                         9  >  4  >  B  >  6
                         ^     ^     ^     ^
                         5  >  0  >  7  >  2
                         ^     ^     ^     ^
                         1  >  8  >  3  >  A
*/
const KeyArray clean_cent = {0,12,4,16,-14,-2,-10,2,14,-16,18,-12};
// meantone
const KeyArray meantone_cent = {0,16,-6,10,-13,3,-19,-3,13,-10,6,-16};

const byte MIN_STUNE     = 0x00; // GS scale tune min. -64 cent
const byte MAX_STUNE     = 0x7F; // GS scale tune max. +63 cent
const byte MEAN_STUNE    = 0x40; // GS scale tune chromatic

const byte SYS_EX        = 0xF0; // system exclusive transmit F0
const byte SYS_EX_NT     = 0xF7; // system exclusive not transmit F0
const byte SYS_EX_END    = 0xF7; // end of system exclusive data

const byte ROLAND        = 0x41;
const byte BROADCAST_DEV = 0x7F; // broadcast device
const byte GS_MODEL_ID   = 0x42;
const byte DATA_SET_1    = 0x12;

const int STUNE_LENGTH = 22;     // length of sysex scale tune string
byte stune[STUNE_LENGTH];        // the scale tune MIDI sysex message

// non-existing MIDI channel 16 is used for addressing patch data
const byte STUNE_PATCH_CHANNEL = 16;

#define DEBUG_STUNE

/**
 * Calculates checksum of Roland scale tune message.
 * @return the checksum
 */
byte Roland_checksum() {
  byte total = 0;
  byte mask = 0x7F;
  for (int i = 5; i <= 19; i++) {
    total += stune[i];
  }
  return (0x80 - (total & mask)) & mask ;
}

/**
 * Sends scale tune message on 1 MIDI channel.
 * @param channel 0..15
 */
void scale_tune_send(byte channel) {
  if ( channel < 9 ) {
    stune[6] = 0x11 + channel;
  }
  else {
    if ( channel > 9 )
      stune[6] = 0x10 + channel;
    else // channel 9
      stune[6] = 0x10 ;
  }
  // GS data set 1 messages have 3 byte address
  stune[STUNE_LENGTH - 2] = Roland_checksum();

  #ifdef DEBUG_STUNE
  Serial.print("scale_tune_send:");
  for (int i = 0; i < STUNE_LENGTH; i++) {
    Serial.print(' ');
    Serial.print(stune[i]);
  }
  Serial.println();
  #endif
}

/**
 * Sends scale tune messages on all MIDI channels.
 */
void scale_tune_send_all() {
  for (byte channel = 0; channel < STUNE_PATCH_CHANNEL; channel++) {
    scale_tune_send(channel);
  }
}

/**
 * Transposes (rotates) scale tune values by 1.
 */
void scale_tune_transpose() {
  byte k = stune[8+11];
  for (int j = 0; j <= 10; j++) {
    stune[8+11-j] = stune[8+11-j-1];
  }
  stune[8] = k ;
}

/**
 * Transposes scale tune values by key and sends them.
 * @param key values are transposed by this value mod 12
 */
void scale_tune_send_key(byte key) {
  key %= N_KEYS;
  for (int i = 1; i <= key; i++) {
    scale_tune_transpose;
  }
  scale_tune_send_all;
}

/**
 * Sets the scale tune of each key to the same value.
 * @param _stune the raw value
 */
void scale_tune_set_equal(byte _stune) {
  for (int i = 0; i <= 11; i++) {
    stune[8+i] = _stune;
  }
}

/**
 * Sets the scale tune of each key to the same value.
 * @param cent the value in cent
 */
void scale_tune_set_equal_cent(int cent) {
  cent = max(cent,-64);
  cent = min(cent,63);
  scale_tune_set_equal(byte(cent + MEAN_STUNE)) ;
}


/**
 * @param 1 data string for all channels together
 */
void scale_tune_set_cent(KeyArray cent) {
  for (int i = 0; i <= 11; i++) {
    cent[i] = max(cent[i],-64);
    cent[i] = min(cent[i],63);
    stune[8+i] = byte(cent[i] + MEAN_STUNE) ;
  }
}

/**
 * Inits the scale tune sysex message with MEAN_STUNE.
 */
void scale_tune_init() {
  stune[0] = SYS_EX;
  stune[1] = ROLAND;
  stune[2] = BROADCAST_DEV;
  stune[3] = GS_MODEL_ID;
  stune[4] = DATA_SET_1;
  stune[5] = 0x40 ;       // address MSB is always equal
                          // stune[6] depends on MIDI channel
  stune[7] = 0x40 ;       // address LSB is always equal
  scale_tune_set_equal(MEAN_STUNE);
  stune[STUNE_LENGTH - 1] = SYS_EX_END;
}







