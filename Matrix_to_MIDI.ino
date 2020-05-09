
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

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midi1);

midi::Channel channel = 1;

/*--------------------------------- setup and main loop ---------------------------------*/

int slice_counter = 0;
const int n_slices = 7;

const int keyboard_led_pin = A2; // = F2 ATMega port+bit

const int no_key = -1;
int last_key = no_key;
int split_position = no_key;
int left_transpose = 0;

const int black_button_pin = 22;
const int green_button_pin = 23;
const int external_switch_pin = A0; // = F0 ATMega port+bit
boolean black_button = false;
boolean green_button = false;
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
  
  // user has 2.5 seconds (re-triggerable timer) to adjust MIDI channel
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
  
  setupMatrixPins();

  readSettings();
  initVelocities(global_sens_to_exponent(settings.sensitivity));

  midi1.begin(1/*dummy input channel*/);
  
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

// report the max. time between calls of scanMatrix, highest observed value: 24 us
//#define DEBUG_EX_SCAN_TIME

State state = idle;

void loop() {
  static int i_led;

  unsigned long t = millis();
  
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
  
  if (!externalSwitch()) {
    buttons(t);
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
    case global_sensitivity: Serial.print("global sens.");   break;
    case key_sensitivity:    Serial.print("key sens.");      break;
  }
  Serial.print(" <- "); 
  switch (event) {
    case up_long:    Serial.println("up long");    break;
    case down_long:  Serial.println("down long");  break;
    case up_short:   Serial.println("up short");   break;
    case down_short: Serial.println("down short"); break;
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
          display(magnify(settings.sensitivity));
          return;
        case down_long:
          digitalWrite(meter_led_pin, LOW);
          state = key_sensitivity;
          last_key = no_key;
          display(meter_max);
          return;
        case up_short: 
        case down_short:
          if (black_button || green_button) {
            // after falling edge still another button pressed, 
            // both must have been pressed together
            if (split_position == no_key) {
              state = wait_for_split;
            }
            else {
              // back to normal mode (no split)
              state = idle;
            }
            split_position = no_key;
          }
          else if (split_position != no_key) {
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
        case up_short: 
        case down_short:
          if (black_button || green_button) {
            // after falling edge still another button pressed, 
            // both must have been pressed together
            // back to normal mode (no split)
            state = idle;
            split_position = no_key;
          }
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
          display(magnify(settings.sensitivity));
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

unsigned long last_switch_time;
const unsigned long long_time = 2500; 

void buttons(unsigned long t_millis) {
  static boolean discard_next_short = false;
  
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
    // falling edge
    black_button = false;
    if (discard_next_short) {
      discard_next_short = false;
    }
    else if (t_millis <= last_switch_time + long_time) {
      process(up_short, -1, -1);
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
    // falling edge
    green_button = false;
    if (discard_next_short) {
      discard_next_short = false;
    }
    else if (t_millis <= last_switch_time + long_time) {
      process(down_short, -1, -1);
    }
  }
}

/*--------------------------------- ext. switch ---------------------------------*/
//#define DEBUG_SUSTAIN
/**
 * Reads external switch value and sends MIDI sustain control change if necessary.
 * @return true on value change
 */
bool externalSwitch() {
  int val = digitalRead(external_switch_pin);
  //int val = (PINF & 0x01 == 0) ? LOW : HIGH;
  if (val == external_switch) {
    // no change
    return false;
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
  return true;
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
    midi1.sendNoteOn(note, velocities[t], chan);
    if (duplicate) {
      midi1.sendNoteOn(note, velocities[t], chan + 1); // this is for my Juno-D, which part of the dual sounds is controlled by expression controller
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
    midi1.sendNoteOff(note, DefaultVelocity, chan);
    if (duplicate) {
      midi1.sendNoteOff(note, DefaultVelocity, chan + 1);
    }
    if (state != idle) {
      process(note_off, key, DefaultVelocity);
    }
}
