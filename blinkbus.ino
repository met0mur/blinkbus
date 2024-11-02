#pragma once
#include "bb_main.h"
#include "bb_hardware.h"

#include <avr/eeprom.h>

BasicHardwareIO hardwareIO; 
BlinkBus facade(&hardwareIO);

//////////////////////////////////////////////////////////////////
////////////////////////  INITIAL SETUP  ///////////////////////
//////////////////////////////////////////////////////////////////

void SetupGesture(int channel, Gesture gtr, Action act, uint8_t map, bool procOrOut = true ) {
  GestureRegister g;
  g.coils.procOrOut = procOrOut;
  g.coils.action = (int)act;
  g.coils.type = (int)gtr;
  g.coils.map = map;
  facade.gestureToSceneMap[channel].Set(g);
}

int GetRegsSize() {
  return sizeof(uint16_t)*registers_count;
}

void LoadFromEeprom() {
  uint8_t flag = eeprom_read_byte(1);

  if (flag == 111) {
    eeprom_read_block((void*)&regs, 10, GetRegsSize());
  } else {
    LoadConfigDefaults();
  }
}

void SaveToEeprom() {
  eeprom_write_byte(1, 111);//mark eeprom initialized
  eeprom_write_block((void*)&regs, 10, GetRegsSize());
}

void LoadConfigDefaults() {
  facade.modbusSlaveId.Set(1);
  facade.modbusSpeed.Set(19200);

  for (int i = 0; i < channel_count; i++) {
    facade.analogToProcMap[i].SetFirstWord( bitWrite(facade.analogToProcMap[i].Get().bytes.first, i, true));
  }
  for (int i = 0; i < channel_count; i++) {
    facade.procToOutputMap[i].SetFirstWord( bitWrite(facade.procToOutputMap[i].Get().bytes.first, i, true));
  }

  facade.lowPassMs.Set(50);
  
  MasterRegister master;
  master.coils.MasterSwitch = 1;
  master.coils.GestureLag = 1;
  facade.master.Set(master);
    
  facade.InvertedGesture.SetWordBit(false, 7, true);

  CommonRegister tg8;
  tg8.bytes.first = 0b00111111;
  facade.analogToGestureMap[7].Set(tg8);

  SetupGesture(0, Gesture::OneClick,    Action::Toggle, 0b00000001);
  SetupGesture(1, Gesture::DoubleClick, Action::Toggle, 0b00000010);
  SetupGesture(2, Gesture::TripleClick, Action::Toggle, 0b00000100);
  SetupGesture(3, Gesture::MediumClick, Action::On,     0b00001000);
  SetupGesture(4, Gesture::Hold,        Action::Toggle, 0b00010000);
  SetupGesture(5, Gesture::DoubleHold,  Action::Toggle, 0b00100000);

  //enable first channel, disable other
  facade.scenes[0].SetFirstWord( 0b00000001);
  //touch 1, 2, 3 channel
  facade.scenes[0].SetSecondWord(0b00000111);

  facade.scenes[1].SetFirstWord( 0b00000010);
  facade.scenes[1].SetSecondWord(0b00000111);

  facade.scenes[2].SetFirstWord( 0b00000100);
  facade.scenes[2].SetSecondWord(0b00000111);

  facade.scenes[3].SetFirstWord( 0b00000111);
  facade.scenes[3].SetSecondWord(0b00000111);

  facade.scenes[4].SetFirstWord( 0b00010000);
  facade.scenes[4].SetSecondWord(0b00010000);

  facade.scenes[5].SetFirstWord( 0b00100000);
  facade.scenes[5].SetSecondWord(0b00100000);

  facade.intervalSmallMs.Set(333);
  facade.intervalBigMs.Set(2000);

  hardwareIO.AnalogLevelThreshold.Set(100);
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

#include <ModbusTCP.h>

Modbus *net;

//arduino setup
void setup() {
  LoadFromEeprom();

  for (int i = 0; i < channel_count; i++) {
      pinMode(hardwareIO.MapOutputPin(i), OUTPUT); 
  } 
  static Modbus netInstance(facade.modbusSlaveId.Get().value, 0, 0);
  net = &netInstance;
  net->begin( 19200, SERIAL_8N2 ); 
}

//arduino loop
void loop() {
  uint16_t data[registers_count];
  memcpy(data, regs, GetRegsSize());
  net->poll( data, registers_count );  
  memcpy(regs, data, GetRegsSize());

  ProcessCommand();
  facade.Process( millis() );
} 

#define register_cmd 0

void ProcessCommand() {
  int cmd = regs[register_cmd].value;
  switch (cmd) {
    case 734:
      SaveToEeprom();
      regs[register_cmd].value = 1;
      break;
    case 2:
      // load defaults
      LoadConfigDefaults();
      regs[register_cmd].value = 1;
      break;
  }
}


