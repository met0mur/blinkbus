#include "bb_primitives.h"
#include "bb_proc.h"
#include "bb_history.h"
#include "bb_register.h"

class BBHardwareIO {
  public:
  virtual bool ReadInput(uint8_t channel) = 0;
  virtual void WriteOutput(uint8_t channel, bool trigger, LightValue lv, uint8_t pwmLevel) = 0;
};

class BlinkBus {
  public:
  BlinkBus() {}

  BlinkBus(BBHardwareIO* ioPtr) {
    io = ioPtr;
  }

  BBHardwareIO* io;

  State<LightValue> analogOutputs[channel_count];
  ZoneProcessor processors[channel_count];
  InputChannelProcessor ChannelProcessor[channel_count];

  SwitchIOModel processorIO{1, true};
  SwitchIOModel processorSensorOut{1, false};
  RegisterModel<MasterRegister> master{2};
  SwitchIOModel analogInputs{3, true};
  SwitchIOModel analogOutputsReg{4, true};

  RegisterModel<CommonRegister> debugger{5};
  RegisterModel<CommonRegister> modbusSlaveId{8};
  RegisterModel<CommonRegister> modbusSpeed{9};

  int currentGestureRotation[channel_count];

  RegisterModel<CommonRegister> analogToProcMap[channel_count] = {
    RegisterModel<CommonRegister>(10),
    RegisterModel<CommonRegister>(11),
    RegisterModel<CommonRegister>(12),
    RegisterModel<CommonRegister>(13),
    RegisterModel<CommonRegister>(14),
    RegisterModel<CommonRegister>(15),
    RegisterModel<CommonRegister>(16),
    RegisterModel<CommonRegister>(17) 
  };

  RegisterModel<CommonRegister> lowPassMs{18};

  RegisterModel<CommonRegister> procToOutputMap[channel_count] = {
    RegisterModel<CommonRegister>(20),
    RegisterModel<CommonRegister>(21),
    RegisterModel<CommonRegister>(22),
    RegisterModel<CommonRegister>(23),
    RegisterModel<CommonRegister>(24),
    RegisterModel<CommonRegister>(25),
    RegisterModel<CommonRegister>(26),
    RegisterModel<CommonRegister>(27) 
  };

  RegisterModel<CommonRegister> intervalSmallMs{28};
  RegisterModel<CommonRegister> intervalBigMs{29};

  RegisterModel<CommonRegister> analogToGestureMap[channel_count] = {
    RegisterModel<CommonRegister>(30),
    RegisterModel<CommonRegister>(31),
    RegisterModel<CommonRegister>(32),
    RegisterModel<CommonRegister>(33),
    RegisterModel<CommonRegister>(34),
    RegisterModel<CommonRegister>(35),
    RegisterModel<CommonRegister>(36),
    RegisterModel<CommonRegister>(37) 
  };

  RegisterModel<GestureRegister> gestureToSceneMap[channel_count] = {
    RegisterModel<GestureRegister>(40),
    RegisterModel<GestureRegister>(41),
    RegisterModel<GestureRegister>(42),
    RegisterModel<GestureRegister>(43),
    RegisterModel<GestureRegister>(44),
    RegisterModel<GestureRegister>(45),
    RegisterModel<GestureRegister>(46),
    RegisterModel<GestureRegister>(47) 
  };

  RegisterModel<SceneActivateRegister> sceneActivation{48};
  //todo remote
  bool sceneActivationHandled = true;

  RegisterModel<CommonRegister> scenes[channel_count] = {
    RegisterModel<CommonRegister>(50),
    RegisterModel<CommonRegister>(51),
    RegisterModel<CommonRegister>(52),
    RegisterModel<CommonRegister>(53),
    RegisterModel<CommonRegister>(54),
    RegisterModel<CommonRegister>(55),
    RegisterModel<CommonRegister>(56),
    RegisterModel<CommonRegister>(57) 
  };

  RegisterModel<CommonRegister> pwmState[channel_count] = {
    RegisterModel<CommonRegister>(60),
    RegisterModel<CommonRegister>(61),
    RegisterModel<CommonRegister>(62),
    RegisterModel<CommonRegister>(63),
    RegisterModel<CommonRegister>(64),
    RegisterModel<CommonRegister>(65),
    RegisterModel<CommonRegister>(66),
    RegisterModel<CommonRegister>(67) 
  };

  //register 90-99 reserved for settings


  void ReadAll() {
    //read analog inputs
    for (int i = 0; i < channel_count; i++) {
      analogInputs.states[i].set(io->ReadInput(i));
    } 

    //read remote registers
    processorIO.Mark();
    processorIO.Read();
  }

  void Process( int32_t currentTime ) {
    ReadAll();

    //check history for gesture
    for (int i = 0; i < channel_count; i++) {

      if ( !ChannelProcessor[i].Inited ) {
        ChannelProcessor[i].Init(
          lowPassMs.get().value, 
          master.get().coils.GestureLag, 
          intervalSmallMs.get().value, 
          intervalBigMs.get().value);
      }

      ChannelProcessor[i].Step( analogInputs.states[i].get() , currentTime );

      //no gestures map
      if (analogToGestureMap[i].get().words.first == 0) {
        continue;
      }

      Gesture g = ChannelProcessor[i].GestureValidate(currentTime);
      if (g != Gesture::Nope) {
        debugger.setFirstWord((int)g);
        //map input to gestures
        forEach8Bit(gestureChannel, analogToGestureMap[i].get().words.first) {
          GestureRegister gesture = gestureToSceneMap[gestureChannel.Get()].get();
          if (gesture.coils.type == (int)g) {
            //first finded gesture set to activate
            SceneActivateRegister sa;
            sa.value = gesture.value;
            sa.coils.rttCh = gestureChannel.Get();
            sceneActivation.set(sa);
            sceneActivationHandled = false;
            break;
          }
        }
      }
    }

    //apply gesture to scene
    if (!sceneActivationHandled) {
      sceneActivationHandled = true;
      SceneActivateRegister sa = sceneActivation.get();

      Action currentAction = static_cast<Action>(sa.coils.action);
      //map matched gesture to output

      Int8RegIterator gtam(sa.coils.map);
      if (sa.coils.rotate && currentGestureRotation[sa.coils.rttCh] > gtam.GetCount() - 1) {
        currentGestureRotation[sa.coils.rttCh] = 0;
      }

      int sceneIndex = 0;
      for (gtam.Reset(); gtam.HasNext(); gtam.Step(), sceneIndex++) {
        
        //skip inactive scenes
        if (sa.coils.rotate && sceneIndex != currentGestureRotation[sa.coils.rttCh] ) {
          continue;
        }

        int sceneChannel = gtam.Get();
        RegisterModel<CommonRegister> sceneMap = scenes[sceneChannel];

        //scene map itetate affected channel
        forEach8Bit(affectedChannel, sceneMap.get().words.second) {
          bool sceneMapValue = sceneMap.getWordBit(true, affectedChannel.Get());

          if (sa.coils.procOrOut) {
            //set action to proccessor
            processors[affectedChannel.Get()].signalGesture.set(sceneMapValue ? currentAction : Action::Off);
          } else {
            //apply action to outputs directly
            LightValue currentValue = analogOutputs[affectedChannel.Get()].get();
            analogOutputs[affectedChannel.Get()].set(sceneMapValue ? ApplyActionToCurrentValue(currentValue, currentAction ) : LightValue::Off);
          }
        }
      }
    }

    //set master states to proc
    for (int i = 0; i < channel_count; i++) {
      processors[i].signalMaster.set(master.get().coils.MasterSwitch);
      processors[i].stateSensorDayMode = master.get().coils.DayMode;
      processors[i].stateSensorEveningMode = master.get().coils.EveningMode;
    }

    //map analog input to processors
    for (int i = 0; i < channel_count; i++) {
      if (!ChannelProcessor[i].FilteredState.hasChanges()) {
        continue;
      }
      //iterate all switch channels
      
      forEach8Bit(procNum, analogToProcMap[i].get().words.first) {
        processors[procNum.Get()].signalSwitch.set(ChannelProcessor[i].FilteredState.get());
      }
      //iterate all sensor channels
      forEach8Bit(procNumS, analogToProcMap[i].get().words.second) {
        processors[procNumS.Get()].signalSensor.set(ChannelProcessor[i].FilteredState.get());
      }

      ChannelProcessor[i].FilteredState.markHandled();
    } 

    //set remote to processors
    for (int i = 0; i < channel_count; i++) {
      if (!processorIO.states[i].hasChanges()) {
        continue;
      }
      processors[i].signalSwitch.set(processorIO.states[i].get());
      processorIO.states[i].markHandled();
    } 

    //run processot and apply results
    for (int i = 0; i < channel_count; i++) {
      ZoneProcessor*processor = &processors[i];
      
      processor->Step();
      processorIO.states[i].set(processor->outputState.get() != LightValue::Off);

      processorSensorOut.states[i].set(
        processor->outputState.get() == LightValue::Half || 
        processor->outputState.get() == LightValue::Min
        );

      if (!processor->outputState.hasChanges()) {
        continue;
      }

      //iterate all enabled channels
      forEach8Bit(analogOutputChannel, procToOutputMap[i].get().words.first) {
        analogOutputs[analogOutputChannel.Get()].set(processor->outputState.get());
      }

      processor->outputState.markHandled();
    }

    WriteAll();
  }

  void WriteAll() {
    for (int i = 0; i < channel_count; i++) {
      LightValue value = analogOutputs[i].get();
      if (analogOutputs[i].hasChanges()) {
        int pwm = 255;
        if (value == LightValue::Off) {
          pwm = 0;
        } else if (value == LightValue::Half) {
          //todo use config
          pwm = 192;
        } else if (value == LightValue::Min) {
          //todo use config
          pwm = 100;
        }
        pwmState[i].set(pwm);
        analogOutputsReg.states[i].set(analogOutputs[i].get() != LightValue::Off);
      }

      io->WriteOutput(i, analogOutputs[i].hasChanges(), value, pwmState[i].get().value);

      analogOutputs[i].markHandled();
    } 

    processorIO.Write();
    analogInputs.Write();
    processorSensorOut.Write();
    analogOutputsReg.Write();
  }
};