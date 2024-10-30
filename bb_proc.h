
class ZoneProcessor {
  public:

  bool StateSensorDayMode;
  bool StateSensorEveningMode;

  Signal<bool> SignalMaster;
  Signal<bool> SignalSwitch;
  Signal<bool> SignalSensor;
  Signal<Action> SignalGesture;

  State<LightValue> OutputState;

  void Step() {
    if (SignalSensor.HasValue()) {
      bool ss = SignalSensor.Use();
      if (ss && m_state == LightValue::Off && !StateSensorDayMode) {
        m_state = StateSensorEveningMode ? LightValue::Half : LightValue::Min;
      } else if (!ss && IsLightValueSemistate(m_state)) {
        m_state = LightValue::Off;
      }
    }

    if (SignalSwitch.HasValue()) {
      bool ssw = SignalSwitch.Use();
      if (ssw && m_state == LightValue::Off) {
        m_state = LightValue::On;
      } else if (!ssw && m_state != LightValue::Off) {
        m_state = LightValue::Off;
      }
    }

    if (SignalMaster.HasValue() && !SignalMaster.Use()) {
      m_state = LightValue::Off;
    }

    if (SignalGesture.HasValue()) {
      m_state = ApplyActionToCurrentValue(m_state, SignalGesture.Use());
    }

    OutputState.Set(m_state);
  }

  private:
  LightValue m_state;
};