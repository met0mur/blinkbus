class BasicHardwareIO : public BBHardwareIO {
  public:

  RegisterModel<CommonRegister> IOFlags{98};
  RegisterModel<CommonRegister> analogLevelThreshold{99};

  //channel number is equals to arduino analog input
  bool ReadInput(uint8_t channel) override {
    return analogRead(channel) > analogLevelThreshold.get().value;
  };

  void WriteOutput(uint8_t channel, bool trigger, LightValue lv, uint8_t pwmLevel) override {
      if (ChannelHasPwm(channel)) {
        analogWrite(MapOutputPin(channel), pwmLevel);
      } else {
        digitalWrite(MapOutputPin(channel), lv != LightValue::Off);
      }
  };

    //channel = 0-7
  int MapOutputPin(int channel) {
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

  int ChannelHasPwm(int channel) {
    switch (channel) {
      case 3:
        return false;
      case 4:
      return false;
    }
    return true;
  }

};