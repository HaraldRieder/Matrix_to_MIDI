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

const int meter_max = 255; // = 3*5*17 = possible deltas
const int meter_pin = 2;
int meter_val;
const int meter_delta = 17;

const int rocker_switch_1_pin = 22;
const int rocker_switch_2_pin = 23;
boolean rocker_switch_1 = false;
boolean rocker_switch_2 = false;

void setup() {
  pinMode(rocker_switch_1_pin, INPUT_PULLUP);
  pinMode(rocker_switch_2_pin, INPUT_PULLUP);
  pinMode(meter_pin, OUTPUT);
  analogWrite(meter_pin, meter_val = meter_max);
  
  setupMatrixPins();

  //ext_switch_1_val = digitalRead(ext_switch_1_pin);

  readGlobals();
  readSensitivities();
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

//State state = playingPreset;
byte * param_value ;

/**
 * The state event machine for the user interface.
 * @param event user action
 * @value optional value, meaning depends on event type
 */
void process(Event event, int value) {
  static int int_param_value;
/*
  switch (state) {

    case editGlobals:
      switch (event) {
        case exitBtn:
          saveGlobals();
          state = showInfo;
          displayInfo();
          return;
        case modWheel:
           value = value * n_global_settings / (MIDI_CONTROLLER_MAX+1);
           if (value != global_parameter) {
             global_parameter = (GlobalParameter)value;
             setParamValuePointer(global_parameter);
             int_param_value = map_from_byte(global_parameter, *param_value);
             displayGlobalParameter(global_parameter, *param_value);
           }
          return;
        case pitchWheel:
          // increment/decrement by 1 or by 10 
          {
            int mini, maxi, range;
            getMinMaxRange(global_parameter, mini, maxi, range);
            if (handlePitchWheelEvent(value, mini, maxi, &int_param_value)) {
              *param_value = map_to_byte(global_parameter, int_param_value);
              displayGlobalParameter(global_parameter, *param_value);
              sendGlobals();
            }
          }
          return;
      }
      return;
      
  }
  */
}

/*--------------------------------- rocker switch ---------------------------------*/

void rockerSwitch() {
  int val = digitalRead(rocker_switch_1_pin);
  if (val == LOW) {
    rocker_switch_1 = true;
  }
  else if (rocker_switch_1) {
    // falling edge
    rocker_switch_1 = false;
    if (meter_val > 0) {
      meter_val -= meter_delta;
    }
    
  }
  val = digitalRead(rocker_switch_2_pin);
  if (val == LOW) {
    rocker_switch_2 = true;
  }
  else if (rocker_switch_2) {
    // falling edge
    rocker_switch_2 = false;
    if (meter_val < meter_max) {
      meter_val += meter_delta;
    }
  }
  analogWrite(meter_pin, meter_val);
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
