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

void setup() {
  //pinMode(push_btn_exit_pin, INPUT_PULLUP);
  
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
  TIMSK3 = 0;
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

//  switch (slice_counter) {
    /*
    case 0:
      inval = digitalRead(push_btn_enter_pin);
      if (inval != push_btn_enter_val) {
        push_btn_enter_val = inval;
        if (inval == HIGH)
          process(enterBtn, inval);
      }
      break;
    case 1:
      inval = digitalRead(push_btn_exit_pin);
      if (inval != push_btn_exit_val) {
        push_btn_exit_val = inval;
        if (inval == HIGH)
          process(exitBtn, inval);
      }
      break;
    case 2:
      inval = digitalRead(ext_switch_1_pin);
      if (inval != ext_switch_1_val) 
        handleExtSwitch1(ext_switch_1_val = inval);
      break;
    case 3:
      inval = digitalRead(ext_switch_2_pin);
      if (inval != ext_switch_2_val)
        handleExtSwitch2(ext_switch_2_val = inval);
      break;
    case 4:
      inval = analogRead(pitch_wheel_pin);
      if (inval > pitch_wheel_val) {
        if (pitch_wheel_up || inval > pitch_wheel_val + wheel_hysteresis) {
          handlePitchWheel(pitch_wheel_val = inval);
          pitch_wheel_up = true;
        }
      } else if (inval < pitch_wheel_val) {
        if (!pitch_wheel_up || inval < pitch_wheel_val - wheel_hysteresis) {
          handlePitchWheel(pitch_wheel_val = inval);
          pitch_wheel_up = false;
        }
      }
      break;
    case 5:
      inval = analogRead(mod_wheel_pin);
      if (inval > mod_wheel_val) {
        if (mod_wheel_up || inval > mod_wheel_val + wheel_hysteresis) {
          handleModWheel(mod_wheel_val = inval);
          mod_wheel_up = true;
        }
      } else if (inval < mod_wheel_val) {
        if (!mod_wheel_up || inval < mod_wheel_val - wheel_hysteresis) {
          handleModWheel(mod_wheel_val = inval);
          mod_wheel_up = false;
        }
      }
      break;
    case 6:
      // reserved
      break;
      */
//  }
    scanMatrix();
  
//  slice_counter++;
//  if (slice_counter >= n_slices)
//    slice_counter = 0;

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

/*--------------------------------- external switches ---------------------------------*/

/**
 * Sends sustain on/off depending on the 
 * input voltage and on whether the external
 * switch (in the foot pedal) is an opener or closer.
 */
 /*
void handleExtSwitch1(int inval) {
  boolean off = ext_switch_1_opener ? (inval == LOW) : (inval == HIGH);
  switch (state) {
    case playingSound:
    case selectSound:
      midi1.sendControlChange(midi::Sustain, off ? 0 : MIDI_CONTROLLER_MAX, sound_channel);
      break;
    default:
      for (int i = 0; i < n_sounds_per_preset; i++) {
        Sound * s = currentPresetSounds[i];
        if (s->ext_switch_1_ctrl_no != NoSwitch)
          midi1.sendControlChange(
            s->ext_switch_1_ctrl_no, 
            off ? (s->ext_switch_1_ctrl_no==Rotor?MIDI_CONTROLLER_MEAN:0) : MIDI_CONTROLLER_MAX, 
            s->channel);
      }
  }
}
*/

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
