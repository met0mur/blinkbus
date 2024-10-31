#pragma once
#include <Arduino.h>

#ifndef registers_count
#define registers_count 100 
#endif

//do not try change this =)
#define channel_count 8 

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

union ZoneToAnalogRegister
{
   struct {
     uint8_t Map :8;
     bool MergeRule :2;//enum MergeRule
     bool reserve    :6;
   } coils;
   struct {
     uint8_t first  :8;
     uint8_t second :8;
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

  RegisterModel(uint8_t reg) {
    m_registerNum = reg;
    }

  T Get() {
    T t;
    t.value = regs[m_registerNum].value;
    return t;
  }

  void Set(T t) {
    regs[m_registerNum].value = t.value;
  }

  void Set(uint16_t t) {
    regs[m_registerNum].value = t;
  }

  void SetFirstWord(uint8_t w) {
    regs[m_registerNum].words.first = w;
  }

  void SetSecondWord(uint8_t w) {
    regs[m_registerNum].words.second = w;
  }

  bool GetWordBit(bool hiLo, uint8_t bit) {
    return bitRead(hiLo ? regs[m_registerNum].words.first : regs[m_registerNum].words.second,bit);
  }

  void SetWordBit(bool hiLo, uint8_t bit, bool value) {
    if (hiLo) {
      regs[m_registerNum].words.first = bitWrite(regs[m_registerNum].words.first,bit, value);
    } else {
      regs[m_registerNum].words.second = bitWrite(regs[m_registerNum].words.second,bit, value);
    }
  }

  protected:
  uint8_t m_registerNum;
};

//
// Read\write registers in defined address by the list of binary states
//

class SwitchIOModel {
  public:

  SwitchIOModel(uint16_t regNumber, bool hiLo) : Reg(regNumber) {
      m_hiLo = hiLo;
  }

  RegisterModel<CommonRegister> Reg;
  State<bool> States[channel_count];

  void virtual Read() {
    for (int i = 0; i < channel_count; i++) {
      int v = m_hiLo ? Reg.Get().words.first : Reg.Get().words.second;
      States[i].Set(bitRead(v,i));
    }
  }

  void virtual Write() {
    int value = 0;
    for (int i = 0; i < channel_count; i++) {
      value = bitWrite(value,i, States[i].Get());
      if (m_hiLo) {
        Reg.SetFirstWord(value);
      } else {
        Reg.SetSecondWord(value);
      }
    }
  }

  void virtual Mark() {
    for (int i = 0; i < channel_count; i++) {
      States[i].MarkHandled();
    }
  }

  protected:
  bool m_hiLo;
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
      m_value = value;
      m_current = 0;
    }
    bool HasNext() {
      if (m_current >= 8 ) {
        return false;
      }
      while (!bitRead(m_value,m_current) && m_current < 8) {
        m_current++;
      }
      if (m_current >= 8 ) {
        return false;
      }
      return bitRead(m_value,m_current);
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

    uint16_t Get() {return m_current;}
    void Step() { m_current++;}
    void Reset() {m_current = 0;}
    void Reset(uint8_t value) {
      m_value = value;
      m_current = 0;
    }
private:
  uint8_t m_value;
  uint8_t m_current = 0;
};

///
/// uint8_t target = 255; 
/// forEach8Bit(target, i) {
///     i.Get();//0-7
/// }
///
#define forEach8Bit(name,source) for (Int8RegIterator name(source); name.HasNext(); name.Step())

//todo
//#define forEach8Bit(name,source) int name = 0; for (Int8RegIterator t##name(source); t##name.HasNext(), name = t##name.Get(); t##name.Step())

//
// Read\write single coil in defined address
//
class CoilModel {
    public:
    CoilModel(uint8_t reg, uint8_t bit) {
      m_registerNum = reg;
      m_bit = bit;
    }

    void Set(bool value) {
      regs[m_registerNum].value = bitWrite(regs[m_registerNum].value, m_bit, value);
    }

    bool Get() {
      return bitRead(regs[m_registerNum].value, m_bit);
    }

    private:
    uint8_t m_registerNum;
    uint8_t m_bit;
};