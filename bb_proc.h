
class ZoneProcessor {
  public:

  bool stateSensorDayMode;
  bool stateSensorEveningMode;

  Signal<bool> signalMaster;
  Signal<bool> signalSwitch;
  Signal<bool> signalSensor;
  Signal<Action> signalGesture;

  State<LightValue> outputState;

  void Step() {
    if (signalSensor.hasValue()) {
      bool ss = signalSensor.get();
      if (ss && _state == LightValue::Off && !stateSensorDayMode) {
        _state = stateSensorEveningMode ? LightValue::Half : LightValue::Min;
      } else if (!ss && IsLightValueSemistate(_state)) {
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