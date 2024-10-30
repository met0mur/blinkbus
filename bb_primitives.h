enum class LightValue : uint8_t {Off = 0, On, Half, Min};

#define IsLightValueSemistate(value) ((value) == LightValue::Min || (value) == LightValue::Half)

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

template <typename T>
class Signal {
  public:
  T Get() {return m_value;}
  T Use() {
    Reset();
    return m_value;
  }
  bool HasValue() {return m_hasValue;}
  void Reset() {m_hasValue = false;}

  void Set(T value) {
    m_hasValue = true;
    m_value = value;
  }

  private:
  bool  m_hasValue;
  T     m_value;
};

template <typename T>
class State {
  public:
  T Get() {return m_value;}
  bool HasChanges() {return m_hasChanges;}
  void MarkHandled() {m_hasChanges = false;}

  void Set(T value) {
    if (m_value != value) {
      m_hasChanges = true;
    }
    m_value = value;
  }

  private:
  bool  m_hasChanges;
  T     m_value;
};