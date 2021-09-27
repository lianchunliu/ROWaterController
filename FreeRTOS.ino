#if CONFIG_FREERTOS_UNICORE
#define ARDUINO_RUNNING_CORE 0
#else
#define ARDUINO_RUNNING_CORE 1
#endif

#include "TM1650.h"
#include <Preferences.h>

#define MOTOR_CMD_INCR 1
#define MOTOR_CMD_DECR 2
#define MOTOR_CMD_FULL_SPEED 3
#define MOTOR_CMD_STOP_START 4

#define MOTOR_MAX_SPEED 3000



TM1650 tm(25,26);

void TaskReadKeys(void *pvParameters);
void TaskSerial(void *pvParameters);
void MotorCmdTask(void *pvParameters);
static void init_motor_driver(void);

static volatile unsigned int motor_speed = 0;
static volatile unsigned int lastSpeed = 0;

Preferences preferences;

QueueHandle_t keyQueue;
QueueHandle_t cmdQueue;

void setup() {
  
  Serial.begin(115200);
  
  while (!Serial) {
    vTaskDelay(1);
  }

  Serial.println("start setup");

  tm.SetBrightness(1);

  
  motor_speed = loadMotorSpeedFromRom();

  init_motor_driver();
  
  ChangeAndDisplaySpeed(0);
  
  keyQueue=xQueueCreate(5, //Queue length
                        sizeof(unsigned char));

  cmdQueue=xQueueCreate(5, //Queue length
                        sizeof(unsigned char));

  if (keyQueue != NULL && cmdQueue != NULL) {
    xTaskCreatePinnedToCore(TaskSerial,// Task function
              "TaskSerial",// Task name
              8192,// Stack size 
              NULL,
              1,// Priority
              NULL,
              ARDUINO_RUNNING_CORE);

    xTaskCreatePinnedToCore(TaskReadKeys, // Task function
              "TaskReadKeys",// Task name
              8192,// Stack size 
              NULL,
              1,// Priority
              NULL,
              ARDUINO_RUNNING_CORE);

    
    xTaskCreatePinnedToCore(MotorCmdTask, // Task function
              "MotorCmdTask",// Task name
              8192,// Stack size 
              NULL,
              1,// Priority
              NULL,
              ARDUINO_RUNNING_CORE);
   
  }
  

}


void loop()
{
  if (lastSpeed != motor_speed) {
    saveMotorSpeedToRom(motor_speed);
    lastSpeed = motor_speed;
    Serial.print("save motor speed: ");
    Serial.println(motor_speed);
  }

  vTaskDelay(1000/portTICK_PERIOD_MS);
  /*
  tm.DisplayNum(8888);

  Serial.print("n=");
  Serial.println(n);

  unsigned char key = tm.ScanKeys();
  Serial.print("key=");
  Serial.println(key, HEX);

  vTaskDelay(1000/portTICK_PERIOD_MS);
  */
}

static unsigned int loadMotorSpeedFromRom(void)
{
  preferences.begin("my-app", false);
  
  unsigned int ret = preferences.getUInt("counter", 0);
  
  return ret;
}

static void saveMotorSpeedToRom(unsigned int s)
{
  preferences.begin("my-app", false);

  preferences.putUInt("counter", s);

  preferences.end();
}

static void init_motor_driver()
{
  ledcSetup(0, 100, 8);
  ledcAttachPin(23,0);
}

static void ChangeMotorSpeed(unsigned int s)
{
  Serial.print("change motor speed to : ");
  Serial.println(s);

  ledcWriteTone(0, s);
  
}


static void ChangeAndDisplaySpeed(int d)
{
  motor_speed = motor_speed + d;
  if (motor_speed < 0) {
        motor_speed = 0;
  }

   if (motor_speed >= MOTOR_MAX_SPEED) {
       motor_speed = MOTOR_MAX_SPEED;
   }

   tm.DisplayNum(motor_speed);
   ChangeMotorSpeed(motor_speed);
}
 
void MotorCmdTask(void *pvParameters){
  (void) pvParameters;
  
  Serial.println("start MotorCmdTask");

  
  unsigned char cmd;

  bool isFullSpeed = false;
  bool isStopped = false;
  long lastKeyPressed = millis();
  
  for (;;){
    
    if(xQueueReceive(cmdQueue,&cmd,portMAX_DELAY) == pdPASS ){
      Serial.print("read cmd: ");
     
      Serial.println(cmd);

      if (cmd == MOTOR_CMD_INCR) {
        if (millis() - lastKeyPressed < 400L) {
           ChangeAndDisplaySpeed(10);
        } else {
          ChangeAndDisplaySpeed(1);
        }
        lastKeyPressed = millis();
      }

      if (cmd == MOTOR_CMD_DECR) {
        if (millis() - lastKeyPressed < 400L) {
           ChangeAndDisplaySpeed(-10);
        } else {
          ChangeAndDisplaySpeed(-1);
        }
        lastKeyPressed = millis();
      }

      if (cmd == MOTOR_CMD_FULL_SPEED) {

        if(isFullSpeed) {
          isFullSpeed = false;
          ChangeAndDisplaySpeed(0);
        } else {
          isFullSpeed = true;
          tm.DisplayNum(MOTOR_MAX_SPEED);
          ChangeMotorSpeed(MOTOR_MAX_SPEED);
        }
      }

      if (cmd == MOTOR_CMD_STOP_START) {
        if(isStopped) {
          isStopped = false;
          ChangeAndDisplaySpeed(0);
        } else {
          isStopped = true;
          tm.DisplayNum(0);
          ChangeMotorSpeed(0);
        }
      }
      
    }
    
    vTaskDelay(1);
  }
}

void TaskSerial(void *pvParameters){
  (void) pvParameters;
  
  Serial.println("start TaskSerial");
  
  unsigned char key;
  for (;;){
    
    if(xQueueReceive(keyQueue,&key, portMAX_DELAY) == pdPASS ){
      Serial.print("read key: ");
     
      Serial.println(key);
      unsigned char cmd = 0;
      
      if (key == TM1650_KEY1) {
        cmd = MOTOR_CMD_INCR;
      }

      if (key == TM1650_KEY2) {
        cmd = MOTOR_CMD_DECR;
      }

      if (key == TM1650_KEY3) {
        cmd = MOTOR_CMD_FULL_SPEED;
      }

      if (key == TM1650_KEY4) {
        cmd = MOTOR_CMD_STOP_START;
      }

      if (cmd) {
        xQueueSend(cmdQueue,&cmd,portMAX_DELAY);
      }
     
    }
    
    vTaskDelay(1);
  }
}


void TaskReadKeys(void *pvParameters)
{
  (void) pvParameters;


  unsigned lastKey = 0;
  long lastTime = millis();
  
  Serial.println("start TaskReadKeys");
  
  for (;;){
    
    unsigned char key = tm.ScanKeys();

  //  Serial.print("key=");
  //  Serial.println(key);
    
    if (key == TM1650_KEY1 || key == TM1650_KEY2 || key == TM1650_KEY3 || key == TM1650_KEY4) {

      long t = millis();
      if (t < lastTime) lastTime = 0;
      
      if (key == lastKey && t - lastTime < 300L) {
        continue; // skip dup key
      }

      lastKey = key;
      lastTime = t;
      xQueueSend(keyQueue,&key,portMAX_DELAY);
    }
    
    vTaskDelay(10/portTICK_PERIOD_MS);
  }

}
