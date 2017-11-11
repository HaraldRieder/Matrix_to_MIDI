#include <midi_Message.h>
#include <midi_Namespace.h>
#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Settings.h>

#include <EEPROM.h>
#include "Matrix.h"
#include "Matrix_to_MIDI.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midi1);

midi::Channel channel = 1;

/*--------------------------------- setup and main loop ---------------------------------*/

int slice_counter = 0;
const int n_slices = 7;

const int led_pin = A3;

const int meter_pin = 2;
const int no_key = -1;
int last_key = no_key;

const int rocker_switch_1_pin = 22;
const int rocker_switch_2_pin = 23;
boolean rocker_switch_1 = false;
boolean rocker_switch_2 = false;

void setup() {
  Serial.begin(9600); // debugging
  pinMode(rocker_switch_1_pin, INPUT_PULLUP);
  pinMode(rocker_switch_2_pin, INPUT_PULLUP);
  pinMode(meter_pin, OUTPUT);
  analogWrite(meter_pin, 0);
  pinMode(led_pin, OUTPUT);
  digitalWrite(led_pin, HIGH);
  
  setupMatrixPins();

  //ext_switch_1_val = digitalRead(ext_switch_1_pin);

  readSettings();
  initVelocities();

  midi1.begin(1/*dummy input channel*/);
  
  // disable timers / avoid jitter
  //TIMSK0 = 0; leave timer 0 enabled so that we still have delay() and millis() but not tone()
  TIMSK1 = 0;
  TIMSK2 = 0;
  //TIMSK3 = 0; we need timer 3 for PWM at pin 2
  TIMSK4 = 0;
  TIMSK5 = 0;
}

// max. time Arduino consumes between 2 calls of loop()
int max_ex_loop_time_ms;
int t_start = -1;

void loop() {
//digitalWrite(led_pin, LOW);delay(300);digitalWrite(led_pin, HIGH);delay(300);digitalWrite(led_pin, LOW);    
//  if (t_start > 0)
//    max_ex_loop_time_ms = max(max_ex_loop_time_ms, millis() - t_start);
  
//  int inval;
  
  // call this often
  //midi1.read();

  switch (slice_counter) {
    case 0:
      rockerSwitch();
      break;
    case 1:  
    case 2:  
    case 3:  
    case 4:  
    case 5:  
    case 6:
      // reserved
      break;
  }
  scanMatrix();
  
  slice_counter++;
  if (slice_counter >= n_slices)
    slice_counter = 0;

//  t_start = millis();
}

/*--------------------------------- state event machine ---------------------------------*/

State state = idle;

/**
 * The state event machine for the user interface.
 * @param event user action
 * @value optional value, meaning depends on event type
 */
void process(Event event, int value) {

  switch (state) {

    case idle:
      switch (event) {
        case up_long:
          digitalWrite(led_pin, LOW);
          state = global_sensitivity;
          last_key = no_key;
          analogWrite(meter_pin, settings.sensitivity);
          return;
        case down_long:
          digitalWrite(led_pin, LOW);
          state = key_sensitivity;
          last_key = no_key;
          analogWrite(meter_pin, meter_max);
          return;
      }
      return;
      
    case global_sensitivity:
      switch (event) {
        case up_short:
          if (settings.sensitivity < meter_max) {
            settings.sensitivity += meter_delta;
          }
          analogWrite(meter_pin, settings.sensitivity);
          return;
        case down_short:
          if (settings.sensitivity > 0) {
            settings.sensitivity -= meter_delta;
          }
          analogWrite(meter_pin, settings.sensitivity);
          return;
        case up_long:
        case down_long:
          // exit
          state = idle;
          saveSettings();
          analogWrite(meter_pin, 0);
          digitalWrite(led_pin, HIGH);
          return;
      }
      return;
      
    case key_sensitivity:  // TODO
      switch (event) {
        case up_short:
          //if (settings.sensitivity < meter_max) {
          //  settings.sensitivity += meter_delta;
          //}
          //analogWrite(meter_pin, settings.sensitivity);
          return;
        case down_short:
          //if (settings.sensitivity > 0) {
          //  settings.sensitivity -= meter_delta;
          //}
          //analogWrite(meter_pin, settings.sensitivity);
          return;
        case up_long:
        case down_long:
          // exit
          state = idle;
          saveSettings();
          analogWrite(meter_pin, 0);
          digitalWrite(led_pin, HIGH);
          return;
      }
      return;
  }  
}

/*--------------------------------- rocker switch ---------------------------------*/

unsigned long last_switch_time;
const unsigned long long_time = 3000; 

void rockerSwitch() {
  int val = digitalRead(rocker_switch_1_pin);
  if (val == LOW) {
    if (!rocker_switch_1) {
      rocker_switch_1 = true;
      last_switch_time = millis();
    }
  }
  else if (rocker_switch_1) {
    // falling edge
    rocker_switch_1 = false;
    process(millis() > last_switch_time + long_time ? up_long : up_short, -1);
  }
  val = digitalRead(rocker_switch_2_pin);
  if (val == LOW) {
    if (!rocker_switch_2) {
      rocker_switch_2 = true;
      last_switch_time = millis();
    }
  }
  else if (rocker_switch_2) {
    // falling edge
    rocker_switch_2 = false;
    process(millis() > last_switch_time + long_time ? down_long : down_short, -1);
  }
}

/*--------------------------------- note  on / note off ---------------------------------*/
/*
void handleNoteOn(byte channel, byte note, byte velocity)
{
  digitalWrite(led_pin, LOW);
  switch (state) {
    case playingSound: 
    case selectSound:
      midi1.sendNoteOn(note, velocity, sound_channel);
      break;
    default: // Preset
      if (currentPreset.split_point == invalid)
        midi1.sendNoteOn(note, velocity, right_channel);
      else 
        midi1.sendNoteOn(note, velocity, note > currentPreset.split_point ? right_channel : left_channel);
      // handle note on with velocity 0 like note off
      if (velocity == 0 && state != playingPreset)
        process(noteEvent, note);
  }
  digitalWrite(led_pin, HIGH);
}

void handleNoteOff(byte channel, byte note, byte velocity)
{
  digitalWrite(led_pin, LOW);
  switch (state) {
    case playingSound: 
    case selectSound:
      midi1.sendNoteOff(note, velocity, sound_channel);
      break;
    default: // Preset
      if (currentPreset.split_point == invalid)
        midi1.sendNoteOff(note, velocity, right_channel);
      else 
        midi1.sendNoteOff(note, velocity, note > currentPreset.split_point ? right_channel : left_channel);
      if (state != playingPreset)
        process(noteEvent, note);
  }
  digitalWrite(led_pin, HIGH);
}
*/
/*--------------------------------- foot pedal ---------------------------------*/

/* lowest key */
const midi::DataByte A = 21;
const midi::DataByte DefaultVelocity = 80;

void handleKeyEvent(int key, byte velocity) {
    midi::DataByte note = (midi::DataByte)(key + A);
    if (velocity) { 
      midi1.sendNoteOn(note, velocity, channel);
//      digitalWrite(led_pin, LOW);
    }
    else {
      midi1.sendNoteOff(note, DefaultVelocity, channel);
//      digitalWrite(led_pin, HIGH);
    }
}
