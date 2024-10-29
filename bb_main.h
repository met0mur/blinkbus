
#define history_size 6
struct InputsStatesHistory {
  bool state;
  uint16_t timing[history_size];
  uint32_t last_time;
  uint32_t last_trigger_time;
  uint32_t current_delta;
  bool handledOnce;
  Gesture selectedGesture;
  uint32_t selectedGestureTime;
};

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

  State<bool> precessedAnalogInputs[channel_count];

  SwitchIOModel processorIO{1, true};
  SwitchIOModel processorSensorOut{1, false};
  RegisterModel<MasterRegister> master{2};
  SwitchIOModel analogInputs{3, true};
  SwitchIOModel analogInputsFiltred{3, false};
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

  //register 90-99 reserved for hardwareIO setup

  ZoneProcessor processors[channel_count];

  void ReadAll() {
    //read analog inputs
    for (int i = 0; i < channel_count; i++) {
      analogInputs.states[i].set(io->ReadInput(i));
    } 

    //read remote registers
    processorIO.Mark();
    processorIO.Read();
  }



InputsStatesHistory list_InputsStatesHistory[channel_count];

//todo inverted input
Gesture gestureValidate(uint16_t channel, bool gestureLag) {

  const uint16_t small = intervalSmallMs.get().value;
  const uint16_t loong = intervalBigMs.get().value;

  InputsStatesHistory history = list_InputsStatesHistory[channel];

  if (history.handledOnce) {
    return Gesture::Nope;
  }

  uint16_t (&t)[history_size] = history.timing;

  Gesture newGesture = Gesture::Nope;

  if (!history.state && 
      t[0] < small && t[1] > small) {
      newGesture = Gesture::OneClick;
  }

  if (!history.state && 
      t[0] < small && t[1] < small && t[2] < small && t[3] > small) {
      newGesture = Gesture::DoubleClick;
  }

  if (!history.state && 
      t[0] < small && t[1] < small && t[2] < small && t[3] < small && t[4] < small && t[5] > small) {
      newGesture = Gesture::TripleClick;
  }

  if (!history.state && 
      t[0] > small && t[0] < loong && t[1] > small) {
      newGesture = Gesture::MediumClick;
  }

  if (history.state && 
      history.current_delta > loong && t[0] < small && t[1] < small && t[2] > small) {
      newGesture = Gesture::DoubleHold;
  }

  if (history.state && 
      history.current_delta > loong && t[0] > small) {
      newGesture = Gesture::Hold;
  }

  uint32_t currentTime = millis();

  if (newGesture != Gesture::Nope && list_InputsStatesHistory[channel].selectedGesture != newGesture) {
    list_InputsStatesHistory[channel].selectedGesture = newGesture;
    list_InputsStatesHistory[channel].selectedGestureTime = currentTime;
  }

  if (!gestureLag || 
        list_InputsStatesHistory[channel].selectedGesture != Gesture::Nope && 
        currentTime - list_InputsStatesHistory[channel].selectedGestureTime > small) {
    list_InputsStatesHistory[channel].handledOnce = true;
    return list_InputsStatesHistory[channel].selectedGesture;
  }

  return Gesture::Nope;
}

  void Process() {

    ReadAll();

    //low pass filtring && save timings    
    processHistory();

    //check history for gesture
    for (int i = 0; i < channel_count; i++) {
      //no gestures map
      if (analogToGestureMap[i].get().words.first == 0) {
        continue;
      }

      Gesture g = gestureValidate(i,master.get().coils.GestureLag);
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
      if (!analogInputsFiltred.states[i].hasChanges()) {
        continue;
      }
      //iterate all switch channels
      
      forEach8Bit(procNum, analogToProcMap[i].get().words.first) {
        processors[procNum.Get()].signalSwitch.set(analogInputsFiltred.states[i].get());
      }
      //iterate all sensor channels
      forEach8Bit(procNumS, analogToProcMap[i].get().words.second) {
        processors[procNumS.Get()].signalSensor.set(analogInputsFiltred.states[i].get());
      }
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
      
      processor->Check();
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

    analogInputsFiltred.Mark();
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

  void test() {
  }

  void processHistory() {
    uint32_t current_time = millis();
    for (int i = 0; i < channel_count; i++) {
      InputsStatesHistory history = list_InputsStatesHistory[i];
      bool current_input = analogInputs.states[i].get();
      uint16_t deltat = current_time - history.last_time;
      uint16_t deltat_trigger = current_time - history.last_trigger_time;

      list_InputsStatesHistory[i].current_delta = deltat;

      //has no changes
      if (history.state == current_input) {
        continue;
      }

      history.last_trigger_time = current_time;

      //hi freq filter
      if (deltat_trigger < lowPassMs.get().value) {
        continue;
      }

      //write history shift
      for (int j = history_size-1; j > 0; j--) {
        history.timing[j] = history.timing[j-1];
      }
      //write history current step
      history.state = current_input;
      history.timing[0] = deltat;
      history.last_time = current_time;
      history.handledOnce = false;
      history.selectedGesture = Gesture::Nope;
      history.selectedGestureTime = 0;
      history.current_delta = 0;
      list_InputsStatesHistory[i] = history;

      analogInputsFiltred.states[i].set(current_input);
  }
}

};
