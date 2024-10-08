
/**************** WARNING ****************

This project runs successfully on Arduino Mega 2560 when built with Arduino 2: 1.0.5+dfsg2-4 IDE.
Chosen board: Arduino Mega 2560 or Mega ADK

This project did out-of-the-box *NOT* run on Arduino Mega 2560 when built with Arduino 1.8.10!
The software crashed and the board booted cyclically.

I found out that the direct port I/O in Matrix.h was responsible. For other reasons
I changed the compiler option to -O3 and this seems to have repaired the 
direct port I/O code, too. See platform.txt.

***************** WARNING *****************/

#include <midi_Message.h>
#include <midi_Namespace.h>
#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Settings.h>

#include <EEPROM.h>
#include "Matrix.h"
#include "Dynamic.h"
#include "Matrix_to_MIDI.h"
#include "ScaleTune.h"
#include "CodingSwitch.h"
#include "V3GrandPianoXXL.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midi1);

midi::Channel channel = 1;

/*--------------------------------- setup and main loop ---------------------------------*/

int slice_counter = 0;
const int n_slices = 6;

const int keyboard_led_pin = A2; // = F2 ATMega port+bit

const int no_key = -1;
int last_key = no_key;
int split_position = no_key;
int left_transpose = 0;

const int black_button_pin = 22;
const int green_button_pin = 23;
const int external_switch_pin = A0; // = F0 ATMega port+bit
const int external_control_pin = A4; 
int external_control_val = 1023; // 0..1023 analog value
int external_val = MIDI_CONTROLLER_MAX; // 0..127 MIDI value
const int volume_control_pin = A5;
int volume_control_val = 1023; // 0..1023 analog value
int volume_val = MIDI_CONTROLLER_MAX; // 0..127 MIDI value
boolean black_button = false; // "up"
boolean green_button = false; // "down"
int external_switch = LOW;

void setup() {
  Serial.begin(9600); // debugging
  pinMode(black_button_pin, INPUT_PULLUP);
  pinMode(green_button_pin, INPUT_PULLUP);
  pinMode(external_switch_pin, INPUT_PULLUP);
  pinMode(meter_pin, OUTPUT);
  pinMode(meter_led_pin, OUTPUT);
  pinMode(keyboard_led_pin, OUTPUT);
  digitalWrite(keyboard_led_pin, HIGH);
  
  // user has 5 seconds (re-triggerable timer) to adjust MIDI channel
  digitalWrite(meter_led_pin, LOW);
  channel = readCodingSwitchValue(A1) + 1; // 0=omni, 1..16 are individual channels, 17=off
  displayChannel(channel);
  for (int i = 1; i <= 20; i++) {
    delay(250);
    byte newChannel = readCodingSwitchValue(A1) + 1;
    displayChannel(newChannel);
    if (newChannel != channel) {
      channel = newChannel;
      i = 0;
    }
  } 
  digitalWrite(meter_led_pin, HIGH);
  display(0);
  // V3 XXL needs these 5 seconds to get ready!
  
  setupMatrixPins();

  readSettings();
  initVelocities(global_sens_to_exponent(settings.sensitivity));

  midi1.begin(1/*dummy input channel*/);
  sendGMReset(midi1);
  sendReverbType(1, midi1); // Room 2
  for (int i = 0; i < 3; i++) {
    midi1.sendControlChange(midi::Effects1, 15, channel+i); // Room 2 -> lower reverb send level

  }
  sendEffectType(0, midi1); // Chorus 1
  flattenEQs(midi1);

  // disable timers / avoid jitter
  //TIMSK0 = 0; leave timer 0 enabled so that we still have delay() and millis() but not tone()
  TIMSK1 = 0;
  TIMSK2 = 0;
  //TIMSK3 = 0; we need timer 3 for PWM at pin 2
  TIMSK4 = 0;
  TIMSK5 = 0;

  Serial.println("ready");
}

// max. time Arduino consumes between 2 calls of loop()
int max_ex_scan_time_us = 0;
int t_start = -1;

// blinking LEDs
const int period = 400/*ms*/;
unsigned long last_blink = 0/*ms*/;
boolean keyboard_led_on = false;

/* 3 dodecimes to select left sounds */
const midi::DataByte right_sounds_start = 12*3;

// report the max. time between calls of scanMatrix, highest observed value: 24 us
//#define DEBUG_EX_SCAN_TIME

State state = idle;

void loop() {
  static int i_led;

  unsigned long t = millis();
  int inval;
  switch (slice_counter) {
    case 0:
      buttons(t);
      break;
    case 2:
      inval = analogRead(volume_control_pin);
      // if changed, with +-2 jitter suppression
      if (inval > volume_control_val + 2 || inval < volume_control_val - 2) {
        volume_control_val = inval;
        inval /= 8;
        if (inval != volume_val) {
          sendMasterVolume(volume_val = inval, midi1);
          //Serial.print("volume val "); Serial.println(volume_val);
        }
      }
      break;
    case 4:
      inval = analogRead(external_control_pin);
      // if changed, with +-2 jitter suppression
      if (inval > external_control_val + 2 || inval < external_control_val - 2) {
        external_control_val = inval;
        if (true) inval = delog(inval)/8; // for logarithmic/audio pedals
        else inval /= 8; // for linear pedals
        if (inval != external_val) {
          externalControl(external_val = inval);
          //Serial.print("external val "); Serial.println(external_val);
        }
      }
      break;
    default:       
      externalSwitch();
  }
  
  switch (state) {
    case idle: case global_sensitivity:
      switch (++i_led) {
        case 10:
          //digitalWrite(keyboard_led_pin, LOW);
          PORTF &= 0xfb;
          keyboard_led_on = true;
          break;
        case 11:
          if (split_position == no_key) {
            //digitalWrite(keyboard_led_pin, HIGH);
            PORTF |= 0x04;
            keyboard_led_on = false;
          }
          i_led = 0;
          break;
      }
      break;
    default:
      if (t >= last_blink + period) {
        last_blink = t;
        process(toggle_led, -1, -1);
      }
  }
  
  #ifdef DEBUG_EX_SCAN_TIME
  if (t_start > 0) {
    int ex_scan_time_us = micros() - t_start;
    if (ex_scan_time_us > max_ex_scan_time_us) {
      max_ex_scan_time_us = ex_scan_time_us;
      Serial.print(max_ex_scan_time_us); Serial.println(" microseconds max. ex. scan");
    }
  }
  // typical max. value: 20 microseconds
  #endif
  
  scanMatrix();

  #ifdef DEBUG_EX_SCAN_TIME
  t_start = micros();
  #endif
  
  slice_counter++;
  if (slice_counter >= n_slices) {
    slice_counter = 0;
  }
}

int delog(int value) {
  static int half = 120;
  if (value <= half) {
    //Serial.print("delog - ");Serial.print(value);Serial.print("->");Serial.println((long)value * 512 / half);
    return (long)value * 512 / half;
  }
  //Serial.print("delog + ");Serial.print(value);Serial.print("->");Serial.println((long)(value - half) * 511 / (1023 - half) + 512);
  return (long)(value - half) * 511 / (1023 - half) + 512;
}

/*--------------------------------- state event machine ---------------------------------*/

//#define DEBUG_STATE_MACHINE
/**
 * The state event machine for the user interface.
 * @param event user action
 * @value optional value, meaning depends on event type
 */
void process(Event event, int value, int value2) {
  
  #ifdef DEBUG_STATE_MACHINE
  switch (state) {
    case idle:               Serial.print("idle");           break;
    case wait_for_split:     Serial.print("wait for split"); break;
    case wait_for_preset:    Serial.print("wait for preset");break;
    case global_sensitivity: Serial.print("global sens.");   break;
    case key_sensitivity:    Serial.print("key sens.");      break;
  }
  Serial.print(" <- "); 
  switch (event) {
    case up_long:    Serial.println("up long");    break;
    case down_long:  Serial.println("down long");  break;
    case up_short:   Serial.println("up short");   break;
    case down_short: Serial.println("down short"); break;
    case both:       Serial.println("both");       break;
    case note_on:    Serial.println("note on");    break;
    case note_off:   Serial.println("note off");   break;
    case toggle_led: Serial.println("toggle led"); break;
  }
  delay(150);
  #endif
  
  if (event == toggle_led) {
    //digitalWrite(keyboard_led_pin, (keyboard_led_on = !keyboard_led_on) ? LOW : HIGH);
    if (keyboard_led_on = !keyboard_led_on) {
      PORTF &= 0xfb;
    } else {
      PORTF |= 0x04;
    }
    return;
  }

  switch (state) {

    case idle:
      switch (event) {
        case up_long:
          digitalWrite(meter_led_pin, LOW);
          state = global_sensitivity;
          last_key = no_key;
          display(settings.sensitivity);
          return;
        case down_long:
          digitalWrite(meter_led_pin, LOW);
          state = key_sensitivity;
          last_key = no_key;
          display(meter_max);
          return;
        case up_short: 
          if (split_position != no_key) {
            // toggle left transpose
            digitalWrite(meter_led_pin, LOW);
            if (left_transpose == 0) {
              left_transpose = 12;
              display(meter_max);
            }
            else {
              left_transpose = 0;
              display(0);
            }
            delay(1500);
            digitalWrite(meter_led_pin, HIGH);
            display(0);
          }
          return;
        case down_short:
          state = wait_for_preset;
          return;
        case both:
          if (split_position == no_key) {
            state = wait_for_split;
          }
          else {
            split_position = no_key;
          }
          return;
      }
      return;
      
    case wait_for_split:
      switch (event) {
        case note_off: 
          split_position = value;
          state = idle;
          //digitalWrite(keyboard_led_pin, HIGH); 
          PORTF |= 0x04;
          keyboard_led_on = true;
          return;
        case both:
          state = idle;
          split_position = no_key;
          return;
      }
      return;

    case wait_for_preset:
      switch (event) {
        case note_off:
          if (split_position == no_key) {
            if (value < n_sounds) {
              sendSound(sounds[value], channel, midi1);
              sendFineTune(0x40, channel, midi1);
              sendFineTune(0x40, channel + 1, midi1);
              sendFineTune(0x40, channel + 2, midi1);
              sendCoarseTune(0x40, channel, midi1);
              sendCoarseTune(0x40, channel + 1, midi1);
              sendCoarseTune(0x40, channel + 2, midi1);
              externalControl(external_val);
              state = idle;
            }
          }
          else {
            if (value < right_sounds_start) {
              // set left sound
              if (left_transpose == 0) {
                if (value < n_bass_sounds) {
                  sendSound(bass_sounds[value], channel, midi1);
                  state = idle;
                }
              } 
              else {
                if (value < n_sounds) {
                  sendSound(sounds[value], channel, midi1);
                  state = idle;
                }
              }
              sendFineTune(0x38, channel, midi1);
              sendCoarseTune(0x40, channel, midi1);
            }
            else {
              value -= right_sounds_start;
              if (value < n_registrations) {
                sendRegistration(registrations[value], channel, midi1);
                externalControl(external_val);
                state = idle;
              }
            }
          }
          return;
        case up_short:
        case down_short:
          state = idle;
          return;
      }
      return;
      
    case global_sensitivity:
      switch (event) {
        case note_on:
          // display calculated MIDI velocity value
          display(value2 << 1);
          break;
        case note_off:
          // display global sensitivity 
          display(settings.sensitivity);
          break;
        case up_short:
          if (settings.sensitivity < meter_max) {
            initVelocities(global_sens_to_exponent(settings.sensitivity += global_delta));
            Serial.print("sens. "); Serial.println(settings.sensitivity);
          }
          display(settings.sensitivity);
          return;
        case down_short:
          if (settings.sensitivity > 0) {
            initVelocities(global_sens_to_exponent(settings.sensitivity -= global_delta));
            Serial.print("sens. "); Serial.println(settings.sensitivity);
          }
          display(settings.sensitivity);
          return;
        case up_long:
        case down_long:
          // exit
          state = idle;
          saveSettings();
          display(0);
          digitalWrite(meter_led_pin, HIGH);
          return;
      }
      return;
      
    case key_sensitivity:
      switch (event) {
        case note_on:
          last_key = value;
          // display calculated MIDI velocity value
          display(value2 << 1);
          break;
        case note_off:
          // display sensitivity of last key
          display(magnify(settings.sensitivities[last_key]));
          break;
        case up_short:
          if (last_key != no_key) {
            if (settings.sensitivities[last_key] < meter_max) {
              settings.sensitivities[last_key] += meter_delta;
              Serial.print("sens. "); Serial.println(settings.sensitivities[last_key]);
            }
            display(magnify(settings.sensitivities[last_key]));
          }
          return;
        case down_short:
          if (last_key != no_key) {
            if (settings.sensitivities[last_key] > 0) {
              settings.sensitivities[last_key] -= meter_delta;
              Serial.print("sens. "); Serial.println(settings.sensitivities[last_key]);
            }
            display(magnify(settings.sensitivities[last_key]));
          }
          return;
        case up_long:
        case down_long:
          // exit
          state = idle;
          saveSettings();
          display(0);
          digitalWrite(meter_led_pin, HIGH);
          return;
      }
      return;
  }  
}

/*--------------------------------- push-buttons ---------------------------------*/

const unsigned long long_time = 2500; 

void buttons(unsigned long t_millis) {
  static boolean discard_next_short = false;
  static unsigned long last_switch_time;
  
  //int val = digitalRead(rocker_switch_1_pin);
  int val = PINA & 0b01;
  if (val == 0) {
    if (!black_button) {
      black_button = true;
      last_switch_time = t_millis;
    }
    else {
      if (t_millis >= last_switch_time + long_time) {
        process(up_long, -1, -1);
        last_switch_time = t_millis;
        discard_next_short = true;
      }
    }
  }
  else if (black_button) {
    // black/up button released
    black_button = false;
    if (discard_next_short) {
      discard_next_short = false;
    }
    else if (t_millis <= last_switch_time + long_time) {
      if (green_button) {
        process(both, -1, -1);
        discard_next_short = true;
      } else {
        process(up_short, -1, -1);
      }
    }
  }
  //val = digitalRead(rocker_switch_2_pin);
  val = PINA & 0b10;
  if (val == 0) {
    if (!green_button) {
      green_button = true;
      last_switch_time = t_millis;
    }
    else {
      if (t_millis >= last_switch_time + long_time) {
        process(down_long, -1, -1);
        last_switch_time = t_millis;
        discard_next_short = true;
      }
    }
  }
  else if (green_button) {
    // green/down button released
    green_button = false;
    if (discard_next_short) {
      discard_next_short = false;
    }
    else if (t_millis <= last_switch_time + long_time) {
      if (black_button) {
        process(both, -1, -1);
        discard_next_short = true;
      } else {
        process(down_short, -1, -1);
      }
    }
  }
}

/*--------------------------------- ext. switch ---------------------------------*/
//#define DEBUG_SUSTAIN
/**
 * Reads external switch value and sends MIDI sustain control change if necessary.
 * @return true on value change
 */
void externalSwitch() {
  int val = digitalRead(external_switch_pin);
  //int val = (PINF & 0x01 == 0) ? LOW : HIGH;
  if (val == external_switch) {
    // no change
    return;
  }
  midi::DataByte ctrlval = (external_switch = val) == HIGH ? 127 : 0;
  if (split_position == no_key) {
    midi1.sendControlChange(midi::Sustain, ctrlval, channel);
  }
  else if (left_transpose != 0) {
    // sustain pedal controls left section
    midi1.sendControlChange(midi::Sustain, ctrlval, channel);
  } 
  else {
    // sustain pedal controls right section
    midi1.sendControlChange(midi::Sustain, ctrlval, channel + 1);
    midi1.sendControlChange(midi::Sustain, ctrlval, channel + 2);
  }
  #ifdef DEBUG_SUSTAIN
  if (external_switch) {
    Serial.println("sustain on");
  }
  else {
    Serial.println("sustain off");
  }    
  #endif
}

void externalControl(int value) {
  if (split_position == no_key) {
    midi1.sendControlChange(midi::ExpressionController, value, channel);
  }
  else {
    // expression pedal controlling 1st right sound
    midi1.sendControlChange(midi::ExpressionController, value, channel + 1);
  }
}

/*--------------------------------- event from matrix ---------------------------------*/

/* lowest key */
const midi::DataByte A = 21;
const midi::DataByte DefaultVelocity = 80;

// report calculated MIDI velocity
//#define DEBUG_VELOCITY

void handleKeyDownEvent(byte key, int t_raw) {
    midi::DataByte note = key + A;
    byte chan;
    boolean duplicate = false;
    if (split_position == no_key) {
      chan = channel;
    } 
    else if (key < split_position) {
      // left section or whole keyboard
      chan = channel;
      note += left_transpose;
    } 
    else {
      // right section
      chan = channel + 1;
      note -= split_position >= 12 ? 12 : 0;
      duplicate = true;
    }
    chan &= 0xf;
    int t = t_raw - settings.sensitivities[key] + meter_mean;
    if (t >= t_max)
      t = t_max - 1;
    else if (t < 0)
      t = 0;
    if (state != wait_for_split && state != wait_for_preset) {
      midi1.sendNoteOn(note, velocities[t], chan);
      if (duplicate) {
        midi1.sendNoteOn(note, velocities[t], chan + 1); 
      }
    }
    #ifdef DEBUG_VELOCITY
    Serial.print("meter_mean: "); Serial.print(meter_mean); Serial.print(" ");
    Serial.print(t_raw); Serial.print("->"); Serial.print(t); Serial.print(" * 128 us -> "); Serial.println(velocities[t]);
    #endif
    if (state != idle) {
      process(note_on, key, velocities[t]);
    }
}

void handleKeyUpEvent(byte key) {
    midi::DataByte note = key + A;
    byte chan;
    boolean layered = false;
    if (split_position == no_key) {
      chan = channel;
    } 
    else if (key < split_position) {
      // left section or whole keyboard
      chan = channel;
      note += left_transpose;
    } 
    else {
      // right section
      chan = channel + 1;
      note -= split_position >= 12 ? 12 : 0;
      layered = true;
    }
    chan &= 0xf;
    midi1.sendNoteOff(note, DefaultVelocity, chan);
    if (layered) {
      midi1.sendNoteOff(note, DefaultVelocity, chan + 1);
    }
    if (state != idle) {
      process(note_off, key, DefaultVelocity);
    }
}
