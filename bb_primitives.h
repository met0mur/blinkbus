enum class LightValue : uint8_t {Off = 0, On, Half, Min};

enum class MergeRule : uint8_t {New = 0, High, Low, Escalate};

enum class Gesture : uint8_t {OneClick = 0, DoubleClick, TripleClick, MediumClick, Hold, DoubleHold, Reserve2, Reserve3, Nope};

enum class Action : uint8_t {Off = 0, On, Half, Min, Toggle, Reserve1, Reserve2, Reserve3};

#define IsLightValueSemistate(value) ((value) == LightValue::Min || (value) == LightValue::Half)

LightValue MaxLightValue(LightValue lastState, LightValue newValue) {
    return (uint8_t)lastState > uint8_t(newValue) ? lastState : newValue;
}

LightValue MergeLightValue(LightValue lastState, LightValue newValue, MergeRule rule) {
    switch (rule) {
      case MergeRule::New:
        return newValue;
      case MergeRule::High:
        return MaxLightValue(lastState, newValue);
      case MergeRule::Low:
        return (uint8_t)lastState > uint8_t(newValue) ? newValue : lastState;
      case MergeRule::Escalate:
        if (lastState == newValue) {
          if (newValue == LightValue::Min) {
            return LightValue::Half;
          }
          if (newValue == LightValue::Half) {
            return LightValue::On;
          }
        }
        return MaxLightValue(lastState, newValue);
    }

    return lastState;
}

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