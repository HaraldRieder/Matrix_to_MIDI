#include <midi_Message.h>
#include <midi_Namespace.h>
#include <MIDI.h>
#include <midi_Defs.h>
#include <midi_Settings.h>

#include <EEPROM.h>
#include "Matrix.h"
#include "Matrix_to_MIDI.h"

MIDI_CREATE_INSTANCE(HardwareSerial, Serial1, midi1);

/* data input indicator (MIDI) */
const int led_pin = 13;

midi::Channel channel = 1;

/*--------------------------------- setup and main loop ---------------------------------*/

int slice_counter = 0;
const int n_slices = 7;

void readPresetDefaultChannels(int presetNumber, Preset & preset) {
  readPreset(presetNumber, preset);
  currentPreset.left.channel = left_channel;
  currentPreset.right.channel = right_channel;
  currentPreset.foot.channel = foot_channel;
}

void setup() {
  pinMode(led_pin, OUTPUT);
  //pinMode(push_btn_exit_pin, INPUT_PULLUP);
  
  setupMatrixPins();

  //ext_switch_1_val = digitalRead(ext_switch_1_pin);

  readGlobals();
  sendGlobals();
  readPresetDefaultChannels(preset_number, currentPreset);
  sendPreset(currentPreset);
  displayPreset(currentPreset, preset_number, false);
  
  // disable timers / avoid MIDI jitter
  //TIMSK0 = 0; leave timer 0 enabled so that we still have delay() and millis() but not tone()
  TIMSK1 = 0;
  TIMSK2 = 0;
  TIMSK3 = 0;
  TIMSK4 = 0;
  TIMSK5 = 0;
}

/* noise supression by hysteresis */
const int wheel_hysteresis = 3;

// max. time Arduino consumes between 2 calls of loop()
int max_ex_loop_time_ms;
int t_start = -1;

void loop() {
  if (t_start > 0)
    max_ex_loop_time_ms = max(max_ex_loop_time_ms, millis() - t_start);
  
  int inval;
  
  // call this often
  midi3.read();
  //midi2.read();
  midi3.read();

  switch (slice_counter) {
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
  }
  midi3.read();
  scanPedal();
  
  slice_counter++;
  if (slice_counter >= n_slices)
    slice_counter = 0;

  // call this often
  midi3.read();
  t_start = millis();
}

/*--------------------------------- state event machine ---------------------------------*/

State state = playingPreset;
Sound * editedSound = &currentPreset.right;
byte * param_value ;

/* in sound mode */
int program_number = 0;
enum SD2Bank SD2_current_bank = SD2Presets;

/**
 * The state event machine for the user interface.
 * @param event user action
 * @value optional value, meaning depends on event type
 */
void process(Event event, int value) {
  static GlobalParameter global_parameter = BassBoostParam;
  static ParameterSet parameter_set = CommonParameters;
  static CommonParameter common_parameter = SplitParam;
  static SoundParameter sound_parameter = BankParam;
  static int int_param_value;

  switch (state) {

    case playingPreset:
      switch (event) {
        case exitBtn:
          state = playingSound;
          sendSound(SD2_current_bank, program_number, sound_channel);
          sendSoundParameter(TransposeParam, MIDI_CONTROLLER_MEAN, sound_channel);
          sendSoundParameter(PanParam, MIDI_CONTROLLER_MEAN, sound_channel);
          displaySound(SD2_current_bank, program_number, false);
          return;
        case enterBtn:
          state = selectPreset;
          sendPreset(currentPreset);
          displayPreset(currentPreset, preset_number, true);
          return;
      }
      return;
      
    case playingSound: 
      switch (event) {
        case exitBtn:
          state = editGlobals;
          display(line1, "Global Settings");
          setParamValuePointer(global_parameter);
          int_param_value = map_from_byte(global_parameter, *param_value);
          displayGlobalParameter(global_parameter, *param_value);
          return;
        case enterBtn:
          state = selectSound;
          sendSound(SD2_current_bank, program_number, sound_channel);
          sendSoundParameter(TransposeParam, MIDI_CONTROLLER_MEAN, sound_channel);
          sendSoundParameter(PanParam, MIDI_CONTROLLER_MEAN, sound_channel);
          displaySound(SD2_current_bank, program_number, true);
          return;
      }
      return;
      
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
      
    case showInfo:
      switch (event) {
        case exitBtn:
          state = playingPreset;
          sendPreset(currentPreset);
          displayPreset(currentPreset, preset_number, false);
          return;
      }
      return;

    case selectSound:
      switch (event) {
        case exitBtn:
          state = playingSound;
          displaySound(SD2_current_bank, program_number, false);
          return;
        case pitchWheel:
          // sound select, increment/decrement by 1 or by 10 
          if (handlePitchWheelEvent(value, 0, MIDI_CONTROLLER_MAX, &program_number)) {
            midi3.sendProgramChange(program_number, sound_channel);
            displaySound(SD2_current_bank, program_number, true);
          }
          return;
        case modWheel:
          // bank select, program number remains unchanged
          value = value * n_SD2_banks / (MIDI_CONTROLLER_MAX+1);
          SD2Bank bank = toSD2Bank(value);
          if (bank != SD2_current_bank) {
            SD2_current_bank = bank;
            sendSound(SD2_current_bank, program_number, sound_channel);
            displaySound(SD2_current_bank, program_number, true);
          }
          return;
      }
      return;

    case selectPreset:
      switch (event) {
        case exitBtn:
          state = playingPreset;
          sendPreset(currentPreset);
          displayPreset(currentPreset, preset_number, false);
          return;
        case enterBtn:
          state = editPreset;
          lcd.noBlink();
          display(line1, "Edit Preset");
          parameter_set = CommonParameters;
          displayParameterSet(line2, currentPreset, parameter_set);
          return;
        case pitchWheel:
          // preset select, increment/decrement by 1 or by 10 
          if (handlePitchWheelEvent(value, 0, n_presets-1, &preset_number)) {
            readPresetDefaultChannels(preset_number, currentPreset);
            sendPreset(currentPreset);
            displayPreset(currentPreset, preset_number, true);
          }
          return;
        case modWheel:
          value = value * n_presets / (MIDI_CONTROLLER_MAX+1);
          if (value != preset_number) {
            preset_number = value;
            readPresetDefaultChannels(preset_number, currentPreset);
            sendPreset(currentPreset);
            displayPreset(currentPreset, preset_number, true);
          }
          return;
      }
      return; 
    
    case editPreset:
      switch (event) {
        case exitBtn:
          if (changed(currentPreset, preset_number)) {
            state = askSavePreset;
            display(line1, "Save changed preset?");
            display(line2, "red=yes black=no");
          }
          else {
            state = selectPreset;
            sendPreset(currentPreset);
            displayPreset(currentPreset, preset_number, true);
          }
          return;
        case enterBtn:
          common_parameter = SplitParam;
          sound_parameter = BankParam;
          switch (parameter_set) {
            case CommonParameters:
              state = editPresetCommon;
              setParamValuePointer(common_parameter);
              int_param_value = map_from_byte(common_parameter, *param_value);
              displayCommonParameter(common_parameter, *param_value);
              break;
            case FootParameters:
              state = editPresetSound;
              editedSound = &currentPreset.foot;
              setParamValuePointer(sound_parameter);
              int_param_value = map_from_byte(sound_parameter, *param_value);
              displaySoundParameter(sound_parameter, *param_value, (SD2Bank)editedSound->bank);
              break;
            case LeftParameters:
              state = editPresetSound;
              editedSound = &currentPreset.left;
              setParamValuePointer(sound_parameter);
              int_param_value = map_from_byte(sound_parameter, *param_value);
              displaySoundParameter(sound_parameter, *param_value, (SD2Bank)editedSound->bank);
              break;
            case RightParameters:
              state = editPresetSound;
              editedSound = &currentPreset.right;
              setParamValuePointer(sound_parameter);
              int_param_value = map_from_byte(sound_parameter, *param_value);
              displaySoundParameter(sound_parameter, *param_value, (SD2Bank)editedSound->bank);
              break;
          }
          displayParameterSet(line1, currentPreset, parameter_set);
          return;
        case modWheel:
           // select parameter set
           {
             value = value * n_parameter_sets / (MIDI_CONTROLLER_MAX+1); // now 0..n_parameter_sets
             // depending on the common parameters we have more or less parameter sets: 
             // common, foot, left, right
             // common, left, right (controller pedal)
             // common, foot, right (no split point)
             // common, right (controller pedal, no split point)
             // => correct value when foot or left are not allowed
             if (currentPreset.split_point == invalid && (ParameterSet)value == LeftParameters)
               value = RightParameters;
             if (currentPreset.pedal_mode == ControllerPedal && (ParameterSet)value == FootParameters)
               value = CommonParameters;
             if (value != parameter_set) {
               parameter_set = (ParameterSet)value;
               displayParameterSet(line2, currentPreset, parameter_set);
             }
           }
          return;
      }
      return; 
      
    case editPresetCommon:
      switch (event) {
        case exitBtn:
          state = editPreset;
          display(line1, "Edit Preset");
          displayParameterSet(line2, currentPreset, parameter_set);
          return;
        case modWheel:
           value = value * n_common_parameters / (MIDI_CONTROLLER_MAX+1);
           if (value != common_parameter) {
             common_parameter = (CommonParameter)value;
             setParamValuePointer(common_parameter);
             int_param_value = map_from_byte(common_parameter, *param_value);
             displayCommonParameter(common_parameter, *param_value);
           }
          return;
        case pitchWheel:
          // split point or pedal mode, increment/decrement by 1 or by 10 
          {
            if (handlePitchWheelEvent(value, -1, MIDI_CONTROLLER_MAX, &int_param_value)) {
              *param_value = map_to_byte(common_parameter, int_param_value);
              displayCommonParameter(common_parameter, *param_value);
            }
          }
          return;
        case noteEvent:
          if (common_parameter == SplitParam) {
            *param_value = (byte)value;
            displayCommonParameter(common_parameter, *param_value);
            delay(1500);
            common_parameter = PedalModeParam;
            setParamValuePointer(common_parameter);
            int_param_value = map_from_byte(common_parameter, *param_value);
            displayCommonParameter(common_parameter, *param_value);
          }
          return;
      }
      return; 
      
    case editPresetSound:
      switch (event) {
        case exitBtn:
          state = editPreset;
          display(line1, "Edit Preset");
          displayParameterSet(line2, currentPreset, parameter_set);
          return;
        case enterBtn:
          return;
        case modWheel:
           value = value * n_sound_parameters / (MIDI_CONTROLLER_MAX+1);
           if (value != sound_parameter) {
             sound_parameter = (SoundParameter)value;
             setParamValuePointer(sound_parameter);
             int_param_value = map_from_byte(sound_parameter, *param_value);
             displaySoundParameter(sound_parameter, *param_value, (SD2Bank)editedSound->bank);
           }
          return;
        case pitchWheel:
          // increment/decrement by 1 or by 10 
          {
            int mini, maxi, range;
            getMinMaxRange(sound_parameter, mini, maxi, range);
            if (handlePitchWheelEvent(value, mini, maxi, &int_param_value)) {
              *param_value = map_to_byte(sound_parameter, int_param_value);
              displaySoundParameter(sound_parameter, *param_value, (SD2Bank)editedSound->bank);
              midi::Channel ch = right_channel;
              if (editedSound == &currentPreset.left)
                ch = left_channel;
              else if (editedSound == &currentPreset.foot)
                ch = foot_channel;
              sendSoundParameter(sound_parameter, *param_value, ch);
            }
          }
          return;
        case noteEvent:
          if (sound_parameter == TransposeParam) {
            int_param_value = value; // remember
            state = waitFor2ndTransposeKey;
            display(line1, "Please press");
            display(line2, "second key!");
          }
          return;
      }
      return; 
      
    case waitFor2ndTransposeKey:
      switch (event) {
        case noteEvent: 
          *param_value = (byte)(value - int_param_value + MIDI_CONTROLLER_MEAN);
        case exitBtn:
          displayParameterSet(line1, currentPreset, parameter_set);
          displaySoundParameter(sound_parameter, *param_value, (SD2Bank)editedSound->bank);
          state = editPresetSound;
      }
      return;
      
    case askSavePreset:
      switch (event) {
        case enterBtn:
          savePreset(preset_number, currentPreset);
          display(line1, "Preset saved!");
          display(line2, "");
          delay(1500);
          state = selectPreset;
          sendPreset(currentPreset);
          displayPreset(currentPreset, preset_number, true);
          return;
        case exitBtn:
          state = selectPreset;
          sendPreset(currentPreset);
          displayPreset(currentPreset, -1/*not in EEPROM*/, true);
          return;
        case pitchWheel:
          // preset select, increment/decrement by 1 or by 10 
          if (handlePitchWheelEvent(value, 0, n_presets-1, &preset_number)) {
            displayDestinationPreset(preset_number);
          }
          return;
        case modWheel:
          value = value * n_presets / (MIDI_CONTROLLER_MAX+1);
          if (value != preset_number) {
            preset_number = value;
            displayDestinationPreset(preset_number);
          }
          return;
      }  
  }
}

void displayInfo() {
  display(line1, "Loop delay [ms]:", max_ex_loop_time_ms);
  display(line2, "Pedal scan [ms]:", max_scan_time_ms);
}

const char * toString(GlobalParameter p) {
  switch (p) {
    case BassBoostParam: return "Bs.Boost";
    case BoostFreqParam: return "Boost Frq.";
    case VelocitySlopeParam: return "Velo.Slope";
    case VelocityOffsetParam: return "Velo.Offs.";
    case FilterVelocitySlopeParam: return "Filt.V.Sl.";
    case FilterVelocityOffsetParam: return "Filt.V.Of.";
  }
  return "unknown global";
}

/**
 * Display 1 global parameter value in edit mode.
 */ 
void displayGlobalParameter(GlobalParameter p, byte value) {
  display(line2left, toString(p));
  display(line2right, value);
}

/**
 * Display the destination preset number when saving a changed preset.
 */
void displayDestinationPreset(int preset_number) {
  display(line2left, "Save to >>");
  display(line2right, preset_number+1);
}

/**
 * Displays "Sound" in line 1 of the LCD.
 * Displays bank name, program number (starting with 1) and program name in line 2.
 */ 
void displaySound(SD2Bank bank, int number, boolean blink) {
  display(line1, "Sound");
  // display bank name and program number starting with 1
  display(line2left, toString(bank), number+1);
  // display program name
  display(line2right, toString(bank, number));
  if (blink) {
    lcd.setCursor(lcd_columns/2 - 1,0);
    lcd.blink();
  }
  else 
    lcd.noBlink();
}

/**
 * Displays "Pres." and preset number (starting with 1) in line 1 on the left of the LCD.
 * Displays program names of right, left, foot in the 3 other areas of the LCD.
 */ 
void displayPreset(const Preset & preset, int number, boolean blink) {
  display(line1, "Preset", number+1);
  if (preset.pedal_mode == BassPedal) 
    display(line1right, toString((SD2Bank)(preset.foot.bank), preset.foot.program_number));
  if (preset.split_point != invalid) {
    display(line2left, toString((SD2Bank)(preset.left.bank), preset.left.program_number));
    display(line2right, toString((SD2Bank)(preset.right.bank), preset.right.program_number));
  }
  else if (preset.right.bank != invalid) 
    display(line2, toString((SD2Bank)(preset.right.bank), preset.right.program_number));
  else 
    display(line2, "Invalid Preset!");
  if (blink) {
    lcd.setCursor(lcd_columns/2 - 1,0);
    lcd.blink();
  }
  else 
    lcd.noBlink();
}

/**
 * Sends bank select and program change MIDI messages on channel.
 * @param channel MIDI channel 1..16
 */
void sendSound(SD2Bank bank, midi::DataByte program_number, midi::Channel channel) {
    midi3.sendControlChange(midi::BankSelect, (midi::DataByte)bank, channel);
    midi3.sendProgramChange(program_number, channel);
    // reset all NRPNs
    midi3.sendControlChange(0x77, 0, channel);
}

/**
 * Sends sound settings of 1 preset part to MIDI.
 */ 
void sendSound(const Sound & sound, midi::Channel channel) {
  sendSound((SD2Bank)sound.bank, sound.program_number, channel);
  sendSoundParameter(TransposeParam, sound.transpose, channel);
  sendSoundParameter(VolumeParam, sound.volume, channel);
  sendSoundParameter(PanParam, sound.pan, channel);
  sendSoundParameter(ReverbParam, sound.reverb_send, channel);
  sendSoundParameter(EffectsParam, sound.effects_send, channel);
  sendSoundParameter(CutoffParam, sound.cutoff_frequency, channel);
  sendSoundParameter(ResonanceParam, sound.resonance, channel);
  sendSoundParameter(ReleaseTimeParam, sound.release_time, channel);
}

/**
 * Sends all sound settings of the given preset to MIDI.
 */
void sendPreset(const Preset & preset) {
  if (preset.pedal_mode == BassPedal)
    sendSound(preset.foot, foot_channel); 
  if (preset.split_point != invalid)
    sendSound(preset.left, left_channel); 
  sendSound(preset.right, right_channel); 
}

/**
 * Display name of editable parameter set. 
 * Names of keyboard sets depend on whether there is a split point in the preset.
 */
void displayParameterSet(DisplayArea area, const Preset & preset, const ParameterSet & set) {
  switch (set) {
    case CommonParameters: return display(area, "Common Parameters");
    case FootParameters: return display(area, "Bass Pedal");
    case LeftParameters: return display(area, preset.split_point == invalid ? "Keyboard" : "Left Keyb. Section");
    case RightParameters: return display(area, preset.split_point == invalid ? "Keyboard" : "Right Keyb. Section");
  }
}

const char * NONE = "(none)";
const char * DFLT = "(default)";

void displayCommonParameter(CommonParameter p, byte value) {
  switch (p) {
    case SplitParam: 
      display(line2left, "Split pos:");
      display(line2right, value);
      break;
    case PedalModeParam:
      display(line2left, "Pdl.mode:");
      display(line2right, value == 0 ? "Bass" : "Controller");
      break;
  }
  if (value == invalid)
    display(line2right, NONE);
}

byte map_to_byte(GlobalParameter p, int value) {
  return (byte)value;
}

int map_from_byte(GlobalParameter p, byte value) {
  return (int)value;
}

byte map_to_byte(CommonParameter p, int value) {
   if (p == PedalModeParam)
     value = min(1,max(0,value));
   return (byte)value;
}

int map_from_byte(CommonParameter p, byte value) {
   return (int)value;
}

byte map_to_byte(SoundParameter p, int value) {
  switch (p) {
    case BankParam:
      return toSD2Bank(value);
    case ModAssign: case PitchAssign:
      switch (value) {
        case 1: return Modulation;
        case 2: return CutoffFrequency;
        case 3: return Resonance;
        case 4: return WhaWhaAmount;
        case 5: return Pitch;
      }
      return NoWheel;
    case Switch1Assign: case Switch2Assign:
      switch (value) {
        case 1: return Sustain;
        case 2: return Sostenuto;
        case 3: return Soft;
        case 4: return WhaWha;
        case 5: return Rotor;
      }
      return NoSwitch;
    default:
      return (byte)value;
  }
}

int map_from_byte(SoundParameter p, byte value) {
  switch (p) {
    case BankParam:
      return toIndex((SD2Bank)value);
    case ModAssign: case PitchAssign:
      switch ((WheelAssignableController)value) {
        case Modulation: return 1;
        case CutoffFrequency: return 2;
        case Resonance: return 3;
        case WhaWhaAmount: return 4;
        case Pitch: return 5;
      }
      return 0;
    case Switch1Assign: case Switch2Assign:
      switch (value) {
        case Sustain: return 1;
        case Sostenuto: return 2;
        case Soft: return 3;
        case WhaWha: return 4;
        case Rotor: return 5;
      }
      return 0;
    default:
      return (int)value; // todo sign always ok?
  }
}

const char * toString(WheelAssignableController c, boolean isPitchWheel) {
  switch (c) {
    case NoWheel: return NONE;
    case Modulation: return "modulation";
    case WhaWhaAmount: return "wha-wha";
    case CutoffFrequency: return "cutoff f.";
    case Resonance: return "resonance"; 
    case Pitch: return "pitch bend";
  }
}

const char * toString(SwitchAssignableController c) {
  switch (c) {
    case NoSwitch: return NONE;
    case Sustain: return "sustain";
    case Sostenuto: return "sostenuto";
    case Soft: return "soft";
    case WhaWha: return "wha-wha";
    case Rotor: return "rotor spd.";
  }
}

void displaySoundParameter(SoundParameter p, byte value, SD2Bank bank) {
  switch (p) {
    case BankParam: 
      return display(line2, "Sound bank:",toString((SD2Bank)value));
    case ProgNoParam: 
      return display(line2, "Program:", toString(bank, value));
    case TransposeParam:
      return display(line2, "Transpose:", (int)value - MIDI_CONTROLLER_MEAN);
    case VolumeParam:
      if (value == MIDI_CONTROLLER_MEAN) {
        display(line2left, "Volume:");
        return display(line2right, DFLT);
      }
      return display(line2, "Volume:", value);
    case PanParam:
      display(line2left, "Pan:");
      if (value == MIDI_CONTROLLER_MEAN)
        return display(line2right, "center");
      if (value > MIDI_CONTROLLER_MEAN)
        return display(line2right, ">>>", value - MIDI_CONTROLLER_MEAN);
      return display(line2right, MIDI_CONTROLLER_MEAN - value, "<<<");
    case ReverbParam:
      if (value == MIDI_CONTROLLER_MEAN) {
        display(line2left, "Rev. send:");
        return display(line2right, DFLT);
      }
      return display(line2, "Reverb send:", value);
    case EffectsParam:    
      if (value == MIDI_CONTROLLER_MEAN) {
        display(line2left, "FX send:");
        return display(line2right, DFLT);
      }
      return display(line2, "FX send:", value);
    case CutoffParam:
      if (value == MIDI_CONTROLLER_MEAN) {
        display(line2left, "Cutoff f.:");
        return display(line2right, DFLT);
      }
      return display(line2, "Cutoff frequ.:", value);
    case ResonanceParam:    
      if (value == MIDI_CONTROLLER_MEAN) {
        display(line2left, "Resonance:");
        return display(line2right, DFLT);
      }
      return display(line2, "Resonance:", value);
    case ReleaseTimeParam:    
      if (value == MIDI_CONTROLLER_MEAN) {
        display(line2left, "Release:");
        return display(line2right, DFLT);
      }
      return display(line2, "Release:", value);
    case ModAssign:
      return display(line2, "Mod.wheel>", toString((WheelAssignableController)value, false));
    case PitchAssign:
      return display(line2, "Ptch.whl.>", toString((WheelAssignableController)value, true));
    case Switch1Assign:
      return display(line2, "Switch 1>", toString((SwitchAssignableController)value));
    case Switch2Assign:
      return display(line2, "Switch 2>", toString((SwitchAssignableController)value));
  }
}

/**
 * Sends all global MIDI settings: SD2 bass boost gain+frequency, ...
 */
void sendGlobals() {
  SD2Message msg = bassBoostMsg(globalSettings.SD2_bass_boost, globalSettings.SD2_boost_freq);
  midi3.sendSysEx(msg.length, msg.buff, true);
  for (int i = 0; i < n_SD2_parts; i++) {
    msg = velocitySlopeMsg(i, globalSettings.SD2_velo_slope);
    midi3.sendSysEx(msg.length, msg.buff, true);
    msg = velocityOffsetMsg(i, globalSettings.SD2_velo_offset);
    midi3.sendSysEx(msg.length, msg.buff, true);
    msg = filtervSlopeMsg(i, globalSettings.SD2_filter_velo_slope);
    midi3.sendSysEx(msg.length, msg.buff, true);
    msg = filtervOffsetMsg(i, globalSettings.SD2_filter_velo_offset);
    midi3.sendSysEx(msg.length, msg.buff, true);
  }  
}

void sendSoundParameter(SoundParameter p, byte value, midi::Channel channel) {
  SD2Message msg;
  switch (p) {
    case BankParam: return midi3.sendControlChange(midi::BankSelect, (midi::DataByte)value, channel);
    case ProgNoParam: return midi3.sendProgramChange(value, channel);
    case TransposeParam:
      msg = toNRPNMsg(CoarseTuning, channel - 1, value);
      midi3.sendSysEx(msg.length, msg.buff, true);
      return;
    case VolumeParam:
      if (value != MIDI_CONTROLLER_MEAN)
        midi3.sendControlChange(midi::ChannelVolume, (midi::DataByte)value, channel);
      return;
    case PanParam: return midi3.sendControlChange(midi::Pan, (midi::DataByte)value, channel);
    case ReverbParam: 
      if (value != MIDI_CONTROLLER_MEAN)
        midi3.sendControlChange(0x5b, value, channel); // SD-2, non-standard controller
      return;
    case EffectsParam: 
      if (value != MIDI_CONTROLLER_MEAN)
        midi3.sendControlChange(0x5d, value, channel); // SD-2, non-standard controller
      return;
    case CutoffParam:
      if (value != MIDI_CONTROLLER_MEAN) {
        msg = toNRPNMsg(TVFCutoff, channel - 1, value);
        midi3.sendSysEx(msg.length, msg.buff, true);
      }
      return;
    case ResonanceParam:    
      if (value != MIDI_CONTROLLER_MEAN) {
        msg = toNRPNMsg(TVFResonance, channel - 1, value);
        midi3.sendSysEx(msg.length, msg.buff, true);
      }
      return;
    case ReleaseTimeParam:
      if (value != MIDI_CONTROLLER_MEAN) {
        msg = toNRPNMsg(EnvReleaseTime, channel - 1, value);
        midi3.sendSysEx(msg.length, msg.buff, true);
      }
  }
}

void setParamValuePointer(GlobalParameter p) {
  switch (p) {
    case BassBoostParam:
      param_value = &globalSettings.SD2_bass_boost;
      break;
    case BoostFreqParam:
      param_value = &globalSettings.SD2_boost_freq;
      break;
    case VelocitySlopeParam:
      param_value = &globalSettings.SD2_velo_slope;
      break;
    case VelocityOffsetParam:
      param_value = &globalSettings.SD2_velo_offset;
      break;
    case FilterVelocitySlopeParam:
      param_value = &globalSettings.SD2_filter_velo_slope;
      break;
    case FilterVelocityOffsetParam:
      param_value = &globalSettings.SD2_filter_velo_offset;
      break;
  }
}

void setParamValuePointer(CommonParameter p) {
  switch (p) {
    case SplitParam: 
      param_value = &currentPreset.split_point;
      break;
    case PedalModeParam:
      param_value = &currentPreset.pedal_mode;
      break;
  }
}

void setParamValuePointer(SoundParameter p) {
  switch (p) {
    case BankParam:
      param_value = &(editedSound->bank);
      break;
    case ProgNoParam:
      param_value = &(editedSound->program_number);
      break;
    case TransposeParam:
      param_value = &(editedSound->transpose);
      break;
    case VolumeParam:
      param_value = &(editedSound->volume);
      break;
    case PanParam:
      param_value = &(editedSound->pan);
      break;
    case ReverbParam:
      param_value = &(editedSound->reverb_send);
      break;
    case EffectsParam:
      param_value = &(editedSound->effects_send);
      break;
    case CutoffParam:
      param_value = &(editedSound->cutoff_frequency);
      break;
    case ResonanceParam:
      param_value = &(editedSound->resonance);
      break;
    case ReleaseTimeParam:
      param_value = &(editedSound->release_time);
      break;
    case ModAssign:
      param_value = &(editedSound->mod_wheel_ctrl_no);
      break;
    case PitchAssign:
      param_value = &(editedSound->pitch_wheel_ctrl_no);
      break;
    case Switch1Assign:
      param_value = &(editedSound->ext_switch_1_ctrl_no);
      break;
    case Switch2Assign:
      param_value = &(editedSound->ext_switch_2_ctrl_no);
      break;
  }
}

void getMinMaxRange(GlobalParameter p, int & mini, int & maxi, int & range) {
  mini = 0;
  range = MIDI_CONTROLLER_MAX + 1;
  switch (p) {
    case BassBoostParam:
      range = max_boost_gain + 1;
      break;
    case BoostFreqParam:
      range = max_boost_freq + 1;
      break;
  }
  maxi = mini + range - 1;
}

void getMinMaxRange(SoundParameter p, int & mini, int & maxi, int & range) {
  mini = 0;
  range = MIDI_CONTROLLER_MAX + 1; 
  switch (p) {
    case BankParam:
      range = n_SD2_banks;
      break;
    case ModAssign:
      range = n_wheel_assignable_ctrls - 1; // without pitch bend
      break;
    case PitchAssign:
      range = n_wheel_assignable_ctrls; // with pitch bend
      break;
    case Switch1Assign:  case Switch2Assign:
      range = n_switch_assignable_ctrls; 
      break;
  }  
  maxi = mini + range - 1;
}

/**
 * Returns true if given preset differs from preset in EEPROM addressed by the preset number.
 * @preset to be compared
 * @preset_number index of unchanged preset in EEPROM
 * @return true if different
 */
boolean changed(const Preset & preset, int preset_number) {
  Preset originalPreset;
  readPresetDefaultChannels(preset_number, originalPreset);
  return (memcmp(&originalPreset, &preset, sizeof(Preset)) != 0);
}

/**
 * increment/decrement target value by 1 or by 10 (depending on change of pb_value)
 * @param value pitch bend value from MIDI_PITCHBEND_MIN to MIDI_PITCHBEND_MAX
 * @param min_value minimum allowed value for target_value
 * @param max_value maximum allowed value for target_value
 * @param target_value to be incremented/decremented
 * @return true if target_value has been changed
 */
boolean handlePitchWheelEvent(int value, const int min_value, const int max_value, int * target_value) {
  static int last_pitch_val = 0;
  boolean changed = false;
  value /= (MIDI_PITCHBEND_MAX*3/10);
  // now value is in [-3,-2,-1,0,1,2,3]
  //display(line1, "value", value);
  switch(value) {
    case -3:
      if (last_pitch_val > value) {
        * target_value = min_value;
        changed = true;
      }
      break;
    case -2:
      if (last_pitch_val > value) {
        * target_value = max(* target_value - 10, min_value);
        changed = true;
      }
      break;
    case -1:
      if (last_pitch_val > value) {
        * target_value = max(* target_value - 1, min_value);
        changed = true;
      }
      break;
    case 1:
      if (last_pitch_val < value) {
        * target_value = min(* target_value + 1, max_value);
        changed = true;
      }
      break;
    case 2:
      if (last_pitch_val < value) {
        * target_value = min(* target_value + 10, max_value);
        changed = true;
      }
      break;
    case 3:
      if (last_pitch_val < value) {
        * target_value = max_value;
        changed = true;
      }
      break;
  }
  last_pitch_val = value;
  return changed;
}

/*--------------------------------- external switches ---------------------------------*/

/**
 * Sends sustain on/off depending on the 
 * input voltage and on whether the external
 * switch (in the foot pedal) is an opener or closer.
 */
void handleExtSwitch1(int inval) {
  boolean off = ext_switch_1_opener ? (inval == LOW) : (inval == HIGH);
  switch (state) {
    case playingSound:
    case selectSound:
      midi3.sendControlChange(midi::Sustain, off ? 0 : MIDI_CONTROLLER_MAX, sound_channel);
      break;
    default:
      for (int i = 0; i < n_sounds_per_preset; i++) {
        Sound * s = currentPresetSounds[i];
        if (s->ext_switch_1_ctrl_no != NoSwitch)
          midi3.sendControlChange(
            s->ext_switch_1_ctrl_no, 
            off ? (s->ext_switch_1_ctrl_no==Rotor?MIDI_CONTROLLER_MEAN:0) : MIDI_CONTROLLER_MAX, 
            s->channel);
      }
  }
}

/**
 * Sends sostenuto on/off depending on the 
 * input voltage and on whether the external
 * switch (in the foot pedal) is an opener or closer.
 */
void handleExtSwitch2(int inval) {
  boolean off = ext_switch_2_opener ? (inval == LOW) : (inval == HIGH);
  switch (state) {
    case playingSound:
    case selectSound:
      midi3.sendControlChange(midi::Sostenuto, off ? 0 : MIDI_CONTROLLER_MAX, sound_channel);
      break;
    default:
      for (int i = 0; i < n_sounds_per_preset; i++) {
        Sound * s = currentPresetSounds[i];
        if (s->ext_switch_2_ctrl_no != NoSwitch)
          midi3.sendControlChange(
            s->ext_switch_2_ctrl_no, 
            off ? (s->ext_switch_2_ctrl_no==Rotor?MIDI_CONTROLLER_MEAN:0) : MIDI_CONTROLLER_MAX, 
            s->channel);
      }  
  }
}

/*--------------------------------- pitch bend wheel ---------------------------------*/

/* values from the A/D converter corresponding to wheel positions */
const unsigned int min_pitch = 376;
const unsigned int min_normal_pitch = 533;
const unsigned int lower_pitch_range = min_normal_pitch - min_pitch;
const unsigned int max_normal_pitch = 554;
const unsigned int max_pitch = 694;
const unsigned int upper_pitch_range = max_pitch - max_normal_pitch;

void handlePitchWheel(unsigned int inval) {
  // force 1 initial MIDI message by initialization with invalid value
  static int pitch = MIDI_PITCHBEND_MAX + 1;
  int ctrlval; // value for low resolution MIDI controllers

  //display(line1, "pitch", inval);
  unsigned long ulval; // avoids unsigned int overflow
  if (inval >= min_normal_pitch) {
    if (inval <= max_normal_pitch) {
      inval = 0;
      ctrlval = MIDI_CONTROLLER_MEAN;
    }
    else {
      ulval = MIDI_CONTROLLER_MAX - MIDI_CONTROLLER_MEAN;
      ctrlval = (ulval * (inval - max_normal_pitch)) / upper_pitch_range + MIDI_CONTROLLER_MEAN;
      ulval = MIDI_PITCHBEND_MAX;
      inval = (ulval * (inval - max_normal_pitch)) / upper_pitch_range;
      if (inval > MIDI_PITCHBEND_MAX) {
        inval = MIDI_PITCHBEND_MAX;
        ctrlval = MIDI_CONTROLLER_MAX;
      }
    }
  } else {
    ulval = MIDI_CONTROLLER_MEAN;
    ctrlval = (ulval * (inval - min_normal_pitch)) / lower_pitch_range;
    ulval = -MIDI_PITCHBEND_MIN;
    inval = (ulval * (min_normal_pitch - inval)) / lower_pitch_range;
    inval = -inval;
    if (inval < MIDI_PITCHBEND_MIN)
      inval = MIDI_PITCHBEND_MIN;
  }
  if (inval != pitch) {
    pitch = inval;
    //display(line1, "pitch ", inval);
    switch (state) {
      case playingSound:
      case selectSound:
        midi3.sendPitchBend(pitch, sound_channel);
        break;
      case playingPreset:
        for (int i = 0; i < n_sounds_per_preset; i++) {
          Sound * s = currentPresetSounds[i];
          switch (s->pitch_wheel_ctrl_no) {
            case NoWheel: 
              break;
            case CutoffFrequency:
              sendSoundParameter(CutoffParam, ctrlval, s->channel);
              break;
            case Resonance:
              sendSoundParameter(ResonanceParam, ctrlval, s->channel);
              break;
            case Pitch:
              midi3.sendPitchBend(pitch, s->channel);
              break;
            default: 
              midi3.sendControlChange(s->mod_wheel_ctrl_no, ctrlval, s->channel);
          }
        }
        break;      
      default:
        process(pitchWheel, pitch);
    }
  }
}

/*--------------------------------- modulation wheel ---------------------------------*/

/* values from the A/D converter corresponding to wheel positions */
const unsigned int min_mod = 368;
const unsigned int max_mod = 650;
const unsigned int mod_range = max_mod - min_mod;
// force 1 initial MIDI message by initialization with invalid value
unsigned int modulation = MIDI_CONTROLLER_MAX + 1;

void handleModWheel(unsigned int inval) {
  //display(line2, "mod raw", inval);
  if (inval < min_mod)
    inval = 0;
  else
    inval -= min_mod;
  inval = (MIDI_CONTROLLER_MAX * inval) / mod_range;
  if (inval > MIDI_CONTROLLER_MAX)
    inval = MIDI_CONTROLLER_MAX;
  // controller value range is smaller than input value range
  // therefore different input values may mean the same controller value
  // avoid unnecessary MIDI messages, only send if controller value has changed
  if (inval != modulation) {
    modulation = inval;
    //display(line2, "mod", inval);
    switch (state) {
      case playingSound:
      case selectSound:
        midi3.sendControlChange(midi::ModulationWheel, modulation, sound_channel);
        break;
      case playingPreset:
        for (int i = 0; i < n_sounds_per_preset; i++) {
          Sound * s = currentPresetSounds[i];
          switch (s->mod_wheel_ctrl_no) {
            case NoWheel: 
              break;
            case CutoffFrequency:
              sendSoundParameter(CutoffParam, modulation, s->channel);
              break;
            case Resonance:
              sendSoundParameter(ResonanceParam, modulation, s->channel);
              break;
            default: 
              midi3.sendControlChange(s->mod_wheel_ctrl_no, modulation, s->channel);
          }
        }
        break;
      default:      
      process(modWheel, modulation);
    }
  }
}

/*--------------------------------- note  on / note off ---------------------------------*/

void handleNoteOn(byte channel, byte note, byte velocity)
{
  digitalWrite(led_pin, LOW);
  switch (state) {
    case playingSound: 
    case selectSound:
      midi3.sendNoteOn(note, velocity, sound_channel);
      break;
    default: // Preset
      if (currentPreset.split_point == invalid)
        midi3.sendNoteOn(note, velocity, right_channel);
      else 
        midi3.sendNoteOn(note, velocity, note > currentPreset.split_point ? right_channel : left_channel);
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
      midi3.sendNoteOff(note, velocity, sound_channel);
      break;
    default: // Preset
      if (currentPreset.split_point == invalid)
        midi3.sendNoteOff(note, velocity, right_channel);
      else 
        midi3.sendNoteOff(note, velocity, note > currentPreset.split_point ? right_channel : left_channel);
      if (state != playingPreset)
        process(noteEvent, note);
  }
  digitalWrite(led_pin, HIGH);
}

/*--------------------------------- foot pedal ---------------------------------*/

/* lowest pedal note (without transpose) */
const midi::DataByte E_flat = 27;
const midi::DataByte PedalVelocity = 80;

void handlePedal(int pedal, boolean on) {
  boolean preset_mode = true;
  switch (state) {
    case playingSound:
    case selectSound:
    case editGlobals:
    case showInfo:
      preset_mode = false;
      break;
  }
  digitalWrite(led_pin, LOW);
  if (preset_mode && currentPreset.pedal_mode == BassPedal) {
    midi::DataByte note = (midi::DataByte)(pedal + E_flat); // transpose done by SD2
    if (on) 
      midi3.sendNoteOn(note, PedalVelocity, foot_channel);
    else 
      midi3.sendNoteOff(note, PedalVelocity, foot_channel);
  }
  else {
    // MIDI Controller Pedal
    int controller = -1;
    int change_preset_or_sound_by = 0;
    byte low_value = 0;
    const char * right_text = "";
    switch (pedal) {
    case 0:
      // soft pedal on/off
      controller = midi::SoftPedal;
      right_text = "Soft";
      break;
    case 1:
      change_preset_or_sound_by = -10;
      break;
    case 2:
      // wha-wha on/off
      controller = WhaWha;
      right_text = "Wha-Wha";
      break;
    case 3:
      change_preset_or_sound_by = -1;
      break;
    case 4:
      // sustain pedal on/off => left split area
      controller = midi::Sustain;
      right_text = "left Sust.";
      break;
    case 5:
      change_preset_or_sound_by = +1;
      break;
    case 6:
      // sostenuto pedal on/off => left split area
      controller = midi::Sostenuto;
      right_text = "left Sost.";
      break;
    case 7:
      change_preset_or_sound_by = +10;
      break;
    case 8:
      // sostenuto pedal on/off => right split area
      controller = midi::Sostenuto;
      right_text = "Sostenuto";
      break;
    case 9:
      // in sound mode cycle banks
      if (state == playingSound || state == selectSound) {
        SD2_current_bank = toSD2Bank(toIndex(SD2_current_bank) + 1);
        sendSound(SD2_current_bank, program_number, sound_channel);
        displaySound(SD2_current_bank, program_number, false);
      }
      break;
    case 10:
      // sustain pedal on/off => right split area
      controller = midi::Sustain;
      right_text = "Sustain";
      break;
    case 11:
      break;
    case 12:
      // portamento on/off
      controller = midi::Portamento;
      right_text = "Portam.";
      break;
    case 13:
      // TODO scale tune with key select
      break;
    case 14:
      // rotor fast/slow (not off)
      controller = Rotor;
      right_text = on ? "Rot. fast":"Rot. slow";
      low_value = 0x40;
      break;
    }
    if (controller > 0) {
      switch (state) {
        case playingSound:
        case selectSound:
          midi3.sendControlChange(controller, on ? MIDI_CONTROLLER_MAX : low_value, sound_channel);
          break;
        default:
          if (currentPreset.split_point == invalid)
            midi3.sendControlChange(controller, on ? MIDI_CONTROLLER_MAX : low_value, right_channel);
          else {
            // depending on the controller type send to left of right channel or both
            switch (controller) {
              case midi::Sustain:
              case midi::Sostenuto:
                if (right_text[0] != 'l')
                  break; // not meant for left area
              case midi::SoftPedal:
              case Rotor:
              case WhaWha:
                midi3.sendControlChange(controller, on ? MIDI_CONTROLLER_MAX : low_value, left_channel);
            }
            switch (controller) {
              case midi::Sustain:
              case midi::Sostenuto:
                if (right_text[0] == 'l')
                  break; // not meant for right area
              case midi::SoftPedal:
              case Rotor:
              case midi::Portamento:
                midi3.sendControlChange(controller, on ? MIDI_CONTROLLER_MAX : low_value, right_channel);
            }
          }
      }
    }
    else if (change_preset_or_sound_by != 0) {
      const char * sign = change_preset_or_sound_by > 0 ? "+" : "-";
      if (on) {
        // display the increment/decrement value
        display(line1right, sign, abs(change_preset_or_sound_by));
        return;
      } 
      else {
        // display the new current sound / preset
        if (state == playingSound) {
          program_number = max(0, min(MIDI_CONTROLLER_MAX, program_number + change_preset_or_sound_by));
          sendSound(SD2_current_bank, program_number, sound_channel);
          displaySound(SD2_current_bank, program_number, false);
        }
        else if (state == playingPreset) {
          preset_number = max(0, min(n_presets, preset_number + change_preset_or_sound_by));
          readPresetDefaultChannels(preset_number, currentPreset);
          sendPreset(currentPreset);
          displayPreset(currentPreset, preset_number, true);
        }
      }
    }
    switch (state) {
      case playingSound:
      case selectSound:
      case playingPreset:
      case selectPreset:
        display(line1right, (on || controller == Rotor) ? right_text : "");
    }
  }
  digitalWrite(led_pin, HIGH);
}
