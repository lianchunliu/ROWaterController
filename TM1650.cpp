/**************
 * TM1650.cpp
 **************/


 #include "TM1650.h"

#define DISPLAY_LED_OFF 0x00
#define CMD_SCAN_KEYS 0x49
#define DISPLAY_ADDRESS 0X68
#define CMD_SYSTEM_CONFIG 0x48

#define DEFAULT_DISPLAY_MODE 0x01

#define DISPLAY_LED_NEGATIVE 0x40


TM1650::TM1650(int scl, int sda)
{
    pin_scl = scl;
    pin_sda = sda;

    pinMode(pin_scl, OUTPUT);
    pinMode(pin_sda, OUTPUT);
};

void TM1650::StartSignal(void)
{
  digitalWrite(pin_sda, HIGH);
  digitalWrite(pin_scl, HIGH);
  digitalWrite(pin_sda, LOW);
  digitalWrite(pin_scl, LOW);
}

void TM1650::StopSignal(void)
{
  digitalWrite(pin_scl, LOW);
  digitalWrite(pin_sda, LOW);
  digitalWrite(pin_scl, HIGH);
  digitalWrite(pin_sda, HIGH);
}

bool TM1650::AckSignal(void)
{
 
  digitalWrite(pin_scl, LOW);
  digitalWrite(pin_sda, LOW);
  digitalWrite(pin_scl, HIGH);
  digitalWrite(pin_scl, LOW);
 
  return 0;
}

bool TM1650::WaitAckSignal(void)
{
 

  digitalWrite(pin_scl, LOW);
  digitalWrite(pin_sda, HIGH);
  pinMode(pin_sda, INPUT);
  
  digitalWrite(pin_scl, HIGH);
  
  
  int n = 0;
  while(digitalRead(pin_sda) == 1) {
    delay(1);
    n++;
    if (n > 100) { // timeout
      
      return 1;
    }
  }

  digitalWrite(pin_scl, LOW);

  pinMode(pin_sda, OUTPUT);
  
  return 0;
}


void TM1650::Write1(void)
{
  digitalWrite(pin_sda, HIGH);
  digitalWrite(pin_scl, LOW);
  digitalWrite(pin_scl, HIGH);
  digitalWrite(pin_scl, LOW);
}

void TM1650::Write0(void)
{
  digitalWrite(pin_sda, LOW);
  digitalWrite(pin_scl, LOW);
  digitalWrite(pin_scl, HIGH);
  digitalWrite(pin_scl, LOW);
}

bool TM1650::WriteByte(unsigned char d)
{
  unsigned char tmp;
 
  tmp = d;
  for (int i = 0; i < 8; i++) {
    if (tmp & 0x80) {
      Write1();
    } else {
      Write0();
    }
    tmp = tmp << 1;
  }

  return 0;
}

bool TM1650::WriteCmdAndData(unsigned char cmd, unsigned char d)
{
    StartSignal();
    WriteByte(cmd);
    
    if (WaitAckSignal()) {
      StopSignal();
      return 1;
    }

    WriteByte(d);
    if (WaitAckSignal()) {
      StopSignal();
      return 1;
    }

    StopSignal();

    return 0;
}

bool TM1650::SetBrightness(unsigned char brightness)
{
   bool err = 0;
   unsigned char cmd = 0;

   switch(brightness) {
      case 8:
        cmd = 0x00;
        break;
      case 1:
        cmd = 0x10;
        break;
      case 2:
        cmd = 0x20;
        break;
      case 3:
        cmd = 0x30;
        break;
      case 4:
        cmd = 0x40;
        break;
      case 5:
        cmd = 0x50;
        break;
      case 6:
        cmd = 0x60;
        break;
      case 7:
        cmd = 0x70;
        break;     
   }

   if (brightness == 0) {
      cmd = 0x00;
   } else {
      cmd = cmd | DEFAULT_DISPLAY_MODE;
   }
   
   return WriteCmdAndData(CMD_SYSTEM_CONFIG, cmd);
};


bool TM1650::DisplayNum(int num)
{

    int i = 0;

    if (num < -999) num = -999;
    if (num > 9999) num = 9999;

    if (num < 0) {
      WriteCmdAndData(DIGI_NUM_ADDR[i], DISPLAY_LED_NEGATIVE);
      i++;
      num = abs(num);
    }
    
    int nums[] = {num / 1000 % 10,num / 100 % 10,num / 10 % 10,num % 10};
    
    
    
    while (i < 3) { // show last number
      if (nums[i] == 0) {
        WriteCmdAndData(DIGI_NUM_ADDR[i], DISPLAY_LED_OFF);
      } else {
        break;
      }
      i++;
    }

    while (i < 4) {
      WriteCmdAndData(DIGI_NUM_ADDR[i], NUM_SEGS[nums[i]]);
      i++;
    }

    return 0;
}

unsigned char TM1650::ReadByte()
{
 
  pinMode(pin_sda, INPUT);
  unsigned char key;
  key = 0;
  
  for (int i = 0; i < 8; i++) {
    digitalWrite(pin_scl, HIGH);
    key <<= 1;
    
    if (digitalRead(pin_sda)) {
      key++;
    }
    digitalWrite(pin_scl, LOW);
    //delayMicroseconds(1);
    vTaskDelay(1);
  }
  
  pinMode(pin_sda, OUTPUT);
  
  return key;
  
}

unsigned char TM1650::ScanKeys()
{

  unsigned char key = 0;
  
  StartSignal();
  WriteByte(CMD_SCAN_KEYS);
  
  bool ret = WaitAckSignal();

 // Serial.print("scan keys ret=");
 // Serial.println(ret);

  key = ReadByte();
  
  
  AckSignal();
  StopSignal();
  return key;
}
