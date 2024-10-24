template <typename T>
class Signal {
  public:
  T get() {return _value;}
  bool hasValue() {return _has_value;}
  void reset() {_has_value = false;}

  void set(T value) {
    _has_value = true;
    _value = value;
  }

  private:
  bool _has_value;
  T _value;
};

template <typename T>
class State {
  public:
  T get() {return _value;}
  bool hasChanges() {return _has_changes;}
  void markHandled() {_has_changes = false;}

  void set(T value) {
    if (_value != value) {
      _has_changes = true;
    }
    _value = value;
  }

  private:
  bool _has_changes;
  T _value;
};

enum class LightValue : uint8_t {Off = 0, On, Half, Min};

enum class Gesture : uint8_t {OneClick = 0, DoubleClick, TripleClick, MediumClick, Hold, DoubleHold, Reserve2, Reserve3, Nope};
enum class Action : uint8_t {Off = 0, On, Half, Min, Toggle, Reserve1, Reserve2, Reserve3};

LightValue ApplyActionToCurrentValue(LightValue currentValue, Action currentAction) {
  switch(currentAction){
    case Action::On:
      return LightValue::On;
    case Action::Off:
      return LightValue::Off;
    case Action::Min:
      return LightValue::Min;
    case Action::Half:
      return LightValue::Half;
    case Action::Toggle:
      return currentValue != LightValue::Off ? LightValue::Off : LightValue::On;
  }
}

class ZoneProcessor {
  public:

  bool stateSensorDayMode;
  bool stateSensorEveningMode;

  Signal<bool> signalMaster;
  Signal<bool> signalSwitch;
  Signal<bool> signalSensor;
  Signal<Action> signalGesture;

  State<LightValue> outputState;

  void Check() {
    if (signalSensor.hasValue()) {
      bool ss = signalSensor.get();
      if (ss && _state == LightValue::Off && !stateSensorDayMode) {
        _state = stateSensorEveningMode ? LightValue::Half : LightValue::Min;
      } else if (!ss && (_state == LightValue::Half || _state == LightValue::Min)) {
        _state = LightValue::Off;
      }
      signalSensor.reset();
    }

    if (signalSwitch.hasValue()) {
      bool ssw = signalSwitch.get();
      if (ssw && _state == LightValue::Off) {
        _state = LightValue::On;
      } else if (!ssw && _state != LightValue::Off) {
        _state = LightValue::Off;
      }
      signalSwitch.reset();
    }

    if (signalMaster.hasValue() && !signalMaster.get()) {
      _state = LightValue::Off;
    }

    if (signalGesture.hasValue()) {
      _state = ApplyActionToCurrentValue(_state, signalGesture.get());
      signalGesture.reset();
    }

    outputState.set(_state);
  }

  private:
  LightValue _state;
};


union MasterRegister
{
   struct {
     bool MasterSwitch:1; // enable\disable all switch
     bool DayMode:1;      // enable\disable all sensors
     bool EveningMode:1;  // enable\disable sensor pwm to 99%
     bool InvertInput:1;
     bool InvertOutput:1;
     bool GestureLag:1;
     bool coil06:1;
     bool coil07:1;
     bool coil08:1;
     bool coil09:1;
     bool coil10:1;
     bool coil11:1;
     bool coil12:1;
     bool coil13:1;
     bool coil14:1;
     bool coil15:1;
   } coils;
   struct {
     uint8_t first:8;
     uint8_t second:8;
   } words;
   uint16_t value;
};

union CommonRegister
{
   struct {
     bool coil00:1;
     bool coil01:1;
     bool coil02:1;
     bool coil03:1;
     bool coil04:1;
     bool coil05:1;
     bool coil06:1;
     bool coil07:1;
     bool coil08:1;
     bool coil09:1;
     bool coil10:1;
     bool coil11:1;
     bool coil12:1;
     bool coil13:1;
     bool coil14:1;
     bool coil15:1;
   } coils;
   struct {
     uint8_t first:8;
     uint8_t second:8;
   } words;
   uint16_t value;
};

union GestureRegister
{
   struct {
     uint8_t action:3;
     bool procOrOut:1;
     bool rotate:1;
     uint8_t type:3;
     uint8_t map:8;
   } coils;
   struct {
     uint8_t first:8;
     uint8_t second:8;
   } words;
   uint16_t value;
};

union SceneActivateRegister
{
   struct {
     uint8_t action:3;
     bool procOrOut:1;
     bool rotate:1;
     bool handled:1;
     uint8_t reserve:2;
     uint8_t map:8;
   } coils;
   struct {
     uint8_t first:8;
     uint8_t second:8;
   } words;
   uint16_t value;
};

#define channel_count 8 
#define registers_count 80 
// массив данных modbus
uint16_t data[registers_count];
CommonRegister regs[registers_count];


template <typename T>
class RegisterModel {
  public:

  RegisterModel(uint8_t reg) {_reg = reg;}

  T get() {
    T t;
    t.value = regs[_reg].value;
    return t;
  }

  void set(T t) {
    regs[_reg].value = t.value;
  }

  void set(uint16_t t) {
    regs[_reg].value = t;
  }

  void setFirstWord(uint8_t w) {
    regs[_reg].words.first = w;
  }

  void setSecondWord(uint8_t w) {
    regs[_reg].words.second = w;
  }

  bool getWordBit(bool hiLo, uint8_t bit) {
    return bitRead(hiLo ? regs[_reg].words.first : regs[_reg].words.second,bit);
  }

  protected:
  uint8_t _reg;
};

class SwitchIOModel {
  public:

  SwitchIOModel(uint16_t regNumber, bool hiLo) : reg(regNumber) {
      _hiLo = hiLo;
  }

  RegisterModel<CommonRegister> reg;
  State<bool> states[channel_count];

  void virtual Read() {
    for (int i = 0; i < channel_count; i++) {
      int v = _hiLo ? reg.get().words.first : reg.get().words.second;
      states[i].set(bitRead(v,i));
    }
  }

  void virtual Write() {
    int value = 0;
    for (int i = 0; i < channel_count; i++) {
      value = bitWrite(value,i, states[i].get());
      if (_hiLo) {
        reg.setFirstWord(value);
      } else {
        reg.setSecondWord(value);
      }
    }
  }

  void virtual Mark() {
    for (int i = 0; i < channel_count; i++) {
      states[i].markHandled();
    }
  }

  protected:
  bool _hiLo;
};

class Int8RegIterator {
public:
    Int8RegIterator(uint8_t value) {
      _value = value;
      _current = 0;
    }
    bool HasNext() {
      if (_current >= 8 ) {
        return false;
      }
      while (!bitRead(_value,_current) && _current < 8) {
        _current++;
      }
      if (_current >= 8 ) {
        return false;
      }
      return bitRead(_value,_current);
    }

    uint8_t GetCount() {
      uint8_t count = 0;
      while (HasNext()) {
        count++;
        Step();
      }
      Reset();
      return count;
    }

    uint16_t Get() {return _current;}
    void Step() { _current++;}
    void Reset() {_current = 0;}
    void Reset(uint8_t value) {
      _value = value;
      _current = 0;
    }
private:
  uint8_t _value;
  uint8_t _current = 0;
};

#include <ModbusTCP.h>

#define register_cmd 0
#define register_output 1

struct OutputStates {
  bool states[channel_count];//0-7
};

#define register_remote_IS 2
#define register_switch_IS 3
#define register_switch_min_time 18
#define register_switch_gesture_maps 30
#define register_gesture_maps 40

//channel = 0-7
int mapOutputPin(int channel) {
  switch (channel) {
    case 0:
      return 3;
    case 1:
      return 5;
    case 2:
      return 6;
    case 3:
      return 7;//*
    case 4:
      return 8;//*
    case 5:
      return 9;
    case 6:
      return 10;
    case 7:
      return 11;
  }
}

int channelHasPwm(int channel) {
  switch (channel) {
    case 3:
      return false;
    case 4:
     return false;
  }
  return true;
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

#define history_size 8
struct InputsStatesHistory {
  bool states[history_size];
 /// bool output_states[history_size];
  uint16_t timing[history_size];
  uint32_t last_time;
  uint32_t last_trigger_time;
  uint32_t current_delta;
  bool handledOnce;
  Gesture selectedGesture;
  uint32_t selectedGestureTime;
};

InputsStatesHistory list_InputsStatesHistory[channel_count];

Gesture gestureValidate(uint16_t channel, bool gestureLag) {

  const uint16_t small = 333;
  const uint16_t loong = 2000;


  InputsStatesHistory history = list_InputsStatesHistory[channel];

  if (history.handledOnce) {
    return Gesture::Nope;
  }

  bool (&s)[history_size] = history.states;
  uint16_t (&t)[history_size] = history.timing;

  bool onceClickPattern = !s[2] && s[1] && !s[0];

  Gesture newGesture = Gesture::Nope;

  if (onceClickPattern && 
      t[0] < small && t[1] > small) {
      newGesture = Gesture::OneClick;
  }

  if (!s[4] && s[3] && onceClickPattern && 
      t[0] < small && t[1] < small && t[2] < small && t[3] > small) {
      newGesture = Gesture::DoubleClick;
  }

    if (!s[6] && s[5] && !s[4] && s[3] && onceClickPattern && 
      t[0] < small && t[1] < small && t[2] < small && t[3] < small && t[4] < small && t[5] > small) {
      newGesture = Gesture::TripleClick;
  }

  if (onceClickPattern && 
      t[0] > small && t[0] < loong && t[1] > small) {
      newGesture = Gesture::MediumClick;
  }

  if (s[4] && !s[3] && s[2] && !s[1] && s[0] && 
      history.current_delta > loong && t[0] < small && t[1] < small && t[2] > small) {
      newGesture = Gesture::DoubleHold;
  }

  if (!s[1] && s[0] && 
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

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

#define ID   1      // адрес ведомого
#define btnPin  2   // номер входа, подключенный к кнопке
#define stlPin  13  // номер выхода индикатора работы
                    // расположен на плате Arduino
#define ledPin  12  // номер выхода светодиода

//Задаём ведомому адрес, последовательный порт, выход управления TX
Modbus slave(ID, 0, 0); 
boolean led;
int8_t state = 0;
unsigned long tempus;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

class Facade {
  public:

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

  RegisterModel<CommonRegister> hiPassMs{18};

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

  ZoneProcessor processors[channel_count];

  void ReadAll() {

    //read analog inputs
    for (int i = 0; i < channel_count; i++) {
      analogInputs.states[i].set(analogRead(i) > 100);
    } 

    //read remote registers
    processorIO.Mark();
    processorIO.Read();
  }

  void Process() {
    
    processHistory();

    for (int i = 0; i < channel_count; i++) {
      //no gestures map
      if (analogToGestureMap[i].get().words.first == 0) {
        continue;
      }

      Gesture g = gestureValidate(i,master.get().coils.GestureLag);
      if (g != Gesture::Nope) {
        debugger.setFirstWord((int)g);

        //map input to gestures
        for (Int8RegIterator atgm(analogToGestureMap[i].get().words.first); atgm.HasNext(); atgm.Step()) {
          int gestureChannel = atgm.Get();
          GestureRegister gesture = gestureToSceneMap[gestureChannel].get();
          if (gesture.coils.type == (int)g) {
            SceneActivateRegister sa;
            sa.value = gesture.value;
            sa.coils.handled = false;
            sceneActivation.set(sa);

            Action currentAction = static_cast<Action>(gesture.coils.action);
            //map matched gesture to output

            Int8RegIterator gtam(gesture.coils.map);
            if (gesture.coils.rotate && currentGestureRotation[gestureChannel] > gtam.GetCount() - 1) {
              currentGestureRotation[gestureChannel] = 0;
            }

            int sceneIndex = 0;
            for (gtam.Reset(); gtam.HasNext(); gtam.Step(), sceneIndex++) {
              
              //skip inactive scenes
              if (gesture.coils.rotate && sceneIndex != currentGestureRotation[gestureChannel] ) {
                continue;
              }

              int sceneChannel = gtam.Get();
              RegisterModel<CommonRegister> sceneMap = scenes[sceneChannel];

              //scene map itetate affected channel
              for (Int8RegIterator sm(sceneMap.get().words.second); sm.HasNext(); sm.Step()) {
                int sceneAffectedChannel = sm.Get();
                bool sceneMapValue = sceneMap.getWordBit(true, sceneAffectedChannel);

                if (gesture.coils.procOrOut) {
                  //set action to proccessor
                  processors[sceneAffectedChannel].signalGesture.set(sceneMapValue ? currentAction : Action::Off);
                } else {
                  //apply action to outputs directly
                  analogOutputs[sceneAffectedChannel].set(sceneMapValue ? ApplyActionToCurrentValue( analogOutputs[sceneAffectedChannel].get(), currentAction ) : LightValue::Off);
                }
              }
            }
          }
        }
      }
    }

    //set master states
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
      for (Int8RegIterator atpm(analogToProcMap[i].get().words.first); atpm.HasNext(); atpm.Step()) {
        processors[atpm.Get()].signalSwitch.set(analogInputsFiltred.states[i].get());
      }
      //iterate all sensor channels
      for (Int8RegIterator atpm(analogToProcMap[i].get().words.second); atpm.HasNext(); atpm.Step()) {
        processors[atpm.Get()].signalSensor.set(analogInputsFiltred.states[i].get());
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
      for (Int8RegIterator ptom(procToOutputMap[i].get().words.first); ptom.HasNext(); ptom.Step()) {
        analogOutputs[ptom.Get()].set(processor->outputState.get());
      }

      processor->outputState.markHandled();
    }

    analogInputsFiltred.Mark();
  }

  void WriteAll() {
    for (int i = 0; i < channel_count; i++) {
      LightValue value = analogOutputs[i].get();

      if (!analogOutputs[i].hasChanges()) {
        continue;
      }

      if (channelHasPwm(i)) {
        int pwm = 255;
        if (value == LightValue::Off) {
          pwm = 0;
        } else if (value == LightValue::Half) {
          pwm = 192;
        }
        analogWrite(mapOutputPin(i), pwm);
        pwmState[i].set(pwm);
      } else {
        digitalWrite(mapOutputPin(i), analogOutputs[i].get() == LightValue::On);
      }

      analogOutputsReg.states[i].set(analogOutputs[i].get() != LightValue::Off);

      analogOutputs[i].markHandled();
    } 

    processorIO.Write();
    analogInputs.Write();
    processorSensorOut.Write();
    analogOutputsReg.Write();
  }

  void test() {
  }

};

Facade facade;

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////
////////////////////////  INITIAL SETUP  ///////////////////////
//////////////////////////////////////////////////////////////////

void load_config_defaults() {
  for (int i = 0; i < channel_count - 1; i++) {
    facade.analogToProcMap[i].setFirstWord( bitWrite(facade.analogToProcMap[i].get().words.first, i, true));
  }
  for (int i = 0; i < channel_count; i++) {
    facade.procToOutputMap[i].setFirstWord( bitWrite(facade.procToOutputMap[i].get().words.first, i, true));
  }

  regs[register_switch_min_time].value = 50;
  MasterRegister master = facade.master.get();
  master.coils.MasterSwitch = 1;
  master.coils.GestureLag = 1;
  facade.master.set(master);
    

  CommonRegister tg8 = facade.analogToGestureMap[7].get();
  tg8.coils.coil00 = 1;
  tg8.coils.coil01 = 1;
  tg8.coils.coil02 = 1;
  tg8.coils.coil03 = 1;
  tg8.coils.coil04 = 1;
  tg8.coils.coil05 = 1;
  facade.analogToGestureMap[7].set(tg8);

  GestureRegister g1 = facade.gestureToSceneMap[0].get();
  g1.coils.procOrOut = true;
  g1.coils.action = (int)Action::Toggle;
  g1.coils.type = (int)Gesture::OneClick;
  g1.coils.map = 1;
  facade.gestureToSceneMap[0].set(g1);

  GestureRegister g2 = facade.gestureToSceneMap[1].get();
  g2.coils.procOrOut = true;
  g2.coils.action = (int)Action::Toggle;
  g2.coils.type = (int)Gesture::DoubleClick;
  g2.coils.map = 2;
  facade.gestureToSceneMap[1].set(g2);

  GestureRegister g3 = facade.gestureToSceneMap[2].get();
  g3.coils.procOrOut = true;
  g3.coils.action = (int)Action::Toggle;
  g3.coils.type = (int)Gesture::TripleClick;
  g3.coils.map = 4;
  facade.gestureToSceneMap[2].set(g3);

  GestureRegister g4 = facade.gestureToSceneMap[3].get();
  g4.coils.procOrOut = true;
  g4.coils.action = (int)Action::On;
  g4.coils.type = (int)Gesture::MediumClick;
  g4.coils.map = 8;
  facade.gestureToSceneMap[3].set(g4);

  GestureRegister g5 = facade.gestureToSceneMap[4].get();
  g5.coils.procOrOut = true;
  g5.coils.action = (int)Action::Toggle;
  g5.coils.type = (int)Gesture::Hold;
  g5.coils.map = 16;
  facade.gestureToSceneMap[4].set(g5);

    GestureRegister g6 = facade.gestureToSceneMap[5].get();
  g6.coils.procOrOut = true;
  g6.coils.action = (int)Action::Min;
  g6.coils.type = (int)Gesture::DoubleHold;
  g6.coils.map = 32;
  facade.gestureToSceneMap[5].set(g6);

  facade.scenes[0].setFirstWord(1);
  facade.scenes[0].setSecondWord(7);

  facade.scenes[1].setFirstWord(2);
  facade.scenes[1].setSecondWord(7);

  facade.scenes[2].setFirstWord(4);
  facade.scenes[2].setSecondWord(7);

  facade.scenes[3].setFirstWord(0);
  facade.scenes[3].setSecondWord(7);

  facade.scenes[4].setFirstWord(16);
  facade.scenes[4].setSecondWord(16);

  facade.scenes[5].setFirstWord(32);
  facade.scenes[5].setSecondWord(32);
}

void load_config_from_registers() {
  for (int i = 0; i < channel_count; i++) {
  }
}


void write_config_to_registers() {
  for (int i = 0; i < channel_count; i++) {
  }
}

//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////


void setup() {
  io_setup();
  slave.begin( 19200, SERIAL_8N2 ); 
  load_config_defaults();
}

void io_setup() {
    for (int i = 0; i < channel_count; i++) {
      pinMode(mapOutputPin(i), OUTPUT); 
    } 
}

uint32_t last_loop_time;
uint32_t loop_id;

void loop() {
  memcpy(data, regs, sizeof(uint16_t)*registers_count);
  state = slave.poll( data, registers_count);  
  io_poll_raw();
  memcpy(regs, data, sizeof(uint16_t)*registers_count);
  facade.ReadAll();
  facade.Process();
  facade.WriteAll();
} 

void io_poll_raw() {

  //protocol debug
  //data[29] = slave.getInCnt();
  //data[30] = slave.getOutCnt();
  //data[31] = slave.getErrCnt();

  //profile
  //data[32+(loop_id%4)] = millis() - last_loop_time;
  //loop_id++;
  //last_loop_time = millis();

  //process command
  int cmd = data[register_cmd];
  switch (cmd) {
    case 33:
      // save eprom
      break;
    case 7:
      facade.test();
      data[register_cmd] = 1;
      break;
    case 2:
      // load defaults
      load_config_defaults();
      data[register_cmd] = 1;
      break;
    case 4:
      // read configs from registers
      load_config_from_registers();
      data[register_cmd] = 1;
      break;
    case 5:
      // write configs to registers
      write_config_to_registers();
      data[register_cmd] = 1;
      break;
  }

}

void processHistory() {

  uint32_t current_time = millis();
  for (int i = 0; i < channel_count; i++) {
    InputsStatesHistory history = list_InputsStatesHistory[i];
    bool current_input = facade.analogInputs.states[i].get();
    uint16_t deltat = current_time - history.last_time;
    uint16_t deltat_trigger = current_time - history.last_trigger_time;

    list_InputsStatesHistory[i].current_delta = deltat;

    //has no changes
    if (history.states[0] == current_input) {
      continue;
    }

    history.last_trigger_time = current_time;

    //hi freq filter
    if (deltat_trigger < data[register_switch_min_time]) {
      continue;
    }

    //write history shift
    for (int j = history_size-1; j > 0; j--) {
      history.states[j] = history.states[j-1];
      history.timing[j] = history.timing[j-1];
    }
    //write history current step
    history.states[0] = current_input;
    history.timing[0] = deltat;
    history.last_time = current_time;
    history.handledOnce = false;
    history.selectedGesture = Gesture::Nope;
    history.selectedGestureTime = 0;
    history.current_delta = 0;
    list_InputsStatesHistory[i] = history;

    facade.analogInputsFiltred.states[i].set(current_input);
  } 
}
