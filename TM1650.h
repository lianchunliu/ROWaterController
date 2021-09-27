/******************
 * TM1650.h
 * 
 *****************/


 #ifndef __TM1650_H__
 #define __TM1650_H__

 #include "Arduino.h"


#define TM1650_MAX_BRIGHT 0x07
#define TM1650_KEY1 0x44
#define TM1650_KEY2 0x4D
#define TM1650_KEY3 0x56
#define TM1650_KEY4 0x5F




 class TM1650
 {
    private:
      int pin_scl;
      int pin_sda;

      unsigned char NUM_SEGS[10] = {0x3f,0x06,0x5b,0x4f,0x66,0x6d,0x7d,0x07,0x7f,0x6f};
      unsigned char DIGI_NUM_ADDR[4] = {0x68, 0x6a,0x6c, 0x6e};
      void StartSignal(void);
      void StopSignal(void);
      bool WaitAckSignal(void);
      bool AckSignal(void);
      void Write1(void);
      void Write0(void);
      bool WriteByte(unsigned char data);
      unsigned char ReadByte(void);
      bool WriteCmdAndData(unsigned char cmd, unsigned char data);
      bool DisplayNum1(unsigned char addr, unsigned char data);
      
    public:
      TM1650(int scl, int sda);
      bool SetBrightness(unsigned char brightness = TM1650_MAX_BRIGHT);
      bool DisplayNum(int num);
      unsigned char ScanKeys();
 };


#endif
