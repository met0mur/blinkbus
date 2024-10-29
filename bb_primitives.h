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