#pragma once
#include "bb_main.h"
#include "bb_hardware.h"

#define register_cmd 0

BasicHardwareIO hardwareIO; 
BlinkBus facade(&hardwareIO);

//////////////////////////////////////////////////////////////////
////////////////////////  INITIAL SETUP  ///////////////////////
//////////////////////////////////////////////////////////////////

void load_config_defaults() {
  for (int i = 0; i < channel_count - 1; i++) {
    facade.analogToProcMap[i].SetFirstWord( bitWrite(facade.analogToProcMap[i].Get().words.first, i, true));
  }
  for (int i = 0; i < channel_count; i++) {
    facade.procToOutputMap[i].SetFirstWord( bitWrite(facade.procToOutputMap[i].Get().words.first, i, true));
  }

  regs[18].value = 50;
  MasterRegister master = facade.master.Get();
  master.coils.MasterSwitch = 1;
  master.coils.GestureLag = 1;
  facade.master.Set(master);
    

  CommonRegister tg8 = facade.analogToGestureMap[7].Get();
  tg8.coils.coil00 = 1;
  tg8.coils.coil01 = 1;
  tg8.coils.coil02 = 1;
  tg8.coils.coil03 = 1;
  tg8.coils.coil04 = 1;
  tg8.coils.coil05 = 1;
  facade.analogToGestureMap[7].Set(tg8);

  GestureRegister g1 = facade.gestureToSceneMap[0].Get();
  g1.coils.procOrOut = true;
  g1.coils.action = (int)Action::Toggle;
  g1.coils.type = (int)Gesture::OneClick;
  g1.coils.map = 1;
  facade.gestureToSceneMap[0].Set(g1);

  GestureRegister g2 = facade.gestureToSceneMap[1].Get();
  g2.coils.procOrOut = true;
  g2.coils.action = (int)Action::Toggle;
  g2.coils.type = (int)Gesture::DoubleClick;
  g2.coils.map = 2;
  facade.gestureToSceneMap[1].Set(g2);

  GestureRegister g3 = facade.gestureToSceneMap[2].Get();
  g3.coils.procOrOut = true;
  g3.coils.action = (int)Action::Toggle;
  g3.coils.type = (int)Gesture::TripleClick;
  g3.coils.map = 4;
  facade.gestureToSceneMap[2].Set(g3);

  GestureRegister g4 = facade.gestureToSceneMap[3].Get();
  g4.coils.procOrOut = true;
  g4.coils.action = (int)Action::On;
  g4.coils.type = (int)Gesture::MediumClick;
  g4.coils.map = 8;
  facade.gestureToSceneMap[3].Set(g4);

  GestureRegister g5 = facade.gestureToSceneMap[4].Get();
  g5.coils.procOrOut = true;
  g5.coils.action = (int)Action::Toggle;
  g5.coils.type = (int)Gesture::Hold;
  g5.coils.map = 16;
  facade.gestureToSceneMap[4].Set(g5);

    GestureRegister g6 = facade.gestureToSceneMap[5].Get();
  g6.coils.procOrOut = true;
  g6.coils.action = (int)Action::On;
  g6.coils.type = (int)Gesture::DoubleHold;
  g6.coils.map = 32;
  facade.gestureToSceneMap[5].Set(g6);

  facade.scenes[0].SetFirstWord(1);
  facade.scenes[0].SetSecondWord(7);

  facade.scenes[1].SetFirstWord(2);
  facade.scenes[1].SetSecondWord(7);

  facade.scenes[2].SetFirstWord(4);
  facade.scenes[2].SetSecondWord(7);

  facade.scenes[3].SetFirstWord(0);
  facade.scenes[3].SetSecondWord(7);

  facade.scenes[4].SetFirstWord(16);
  facade.scenes[4].SetSecondWord(16);

  facade.scenes[5].SetFirstWord(32);
  facade.scenes[5].SetSecondWord(32);

  facade.intervalSmallMs.Set(333);
  facade.intervalBigMs.Set(2000);

  hardwareIO.AnalogLevelThreshold.Set(100);
}


//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////

#define ID   1

#include <ModbusTCP.h>
Modbus *net;

//arduino setup
void setup() {
  for (int i = 0; i < channel_count; i++) {
      pinMode(hardwareIO.MapOutputPin(i), OUTPUT); 
  } 
  static Modbus netInstance(ID, 0, 0);
  net = &netInstance;
  net->begin( 19200, SERIAL_8N2 ); 

  load_config_defaults();
}

//arduino loop
void loop() {
  uint16_t data[registers_count];
  memcpy(data, regs, sizeof(uint16_t)*registers_count);
  net->poll( data, registers_count );  
  memcpy(regs, data, sizeof(uint16_t)*registers_count);

  io_poll_raw();
  facade.Process( millis() );
} 

void io_poll_raw() {

  //protocol debug
  //data[29] = slave.getInCnt();
  //data[30] = slave.getOutCnt();
  //data[31] = slave.getErrCnt();


  //process command
  int cmd = regs[register_cmd].value;
  switch (cmd) {
    case 734:
      // save eprom
      regs[register_cmd].value = 1;
      break;
    case 2:
      // load defaults
      load_config_defaults();
      regs[register_cmd].value = 1;
      break;
  }
}


