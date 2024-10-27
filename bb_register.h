#pragma once
#include <Arduino.h>

#ifndef registers_count
#define registers_count 80 
#endif

///
/// Easy way to manage modbus register - union
/// @value, @words field must be in all variants for templete working
///

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

union MasterRegister
{
   struct {
     bool MasterSwitch:1; // enable\disable all switch
     bool DayMode     :1; // enable\disable all sensors
     bool EveningMode :1; // enable\disable sensor pwm to 99%
     bool InvertInput :1;
     bool InvertOutput:1;
     bool GestureLag  :1;
     bool AnimatePWM  :1;
     bool SceneAct    :1;
     bool coil08      :1;
     bool coil09      :1;
     bool coil10      :1;
     bool coil11      :1;
     bool coil12      :1;
     bool coil13      :1;
     bool coil14      :1;
     bool coil15      :1;
   } coils;
   struct {
     uint8_t first    :8;
     uint8_t second   :8;
   } words;
   uint16_t value;
};


union GestureRegister
{
   struct {
     uint8_t action :3;   //enum Action
     bool procOrOut :1;
     bool rotate    :1;   //select one from map sequentially
     uint8_t type   :3;   //enum Gesture
     uint8_t map    :8;
   } coils;
   struct {
     uint8_t first  :8;
     uint8_t second :8;
   } words;
   uint16_t value;
};

union SceneActivateRegister
{
   struct {
     uint8_t action :3;   //enum Action
     bool procOrOut :1;   
     bool rotate    :1;   //select one from map sequentially
     uint8_t rttCh  :3;
     uint8_t map    :8;
   } coils;
   struct {
     uint8_t first  :8;
     uint8_t second :8;
   } words;
   uint16_t value;
};


#ifndef regs
CommonRegister unionRegisters[registers_count]; 
#define regs unionRegisters
#endif

//
// Read\write registers with custom union type in defined address
//

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

//
// Read\write registers in defined address by the list of binary states
//

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

///
/// iterate enabled bits number
/// for example:
///
/// 10100010
/// ^ ^   ^
/// 2 1   0
/// 
///
/// 00000000
/// nope
///
///
/// 11111111
/// ^^^^^^^^
/// 76543210
///

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

/// uint8_t target = 255; 
/// forEach8Bit(target, i) {
///     i.Get();
/// }
///
#define forEach8Bit(name,source) for (Int8RegIterator name(source); name.HasNext(); name.Step())

//todo
//#define forEach8Bit(name,source) int name = 0; for (Int8RegIterator t##name(source); t##name.HasNext(), name = t##name.Get(); t##name.Step())
