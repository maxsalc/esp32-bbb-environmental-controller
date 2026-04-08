#include <Arduino.h>
#include <ESP32Servo.h>
#include <DHT.h>

//#include "scannerstates.h"

HardwareSerial BBBSerial(2);

Servo myServo;
#define DHTPIN 19 //gpio 19 on esp
#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);

//------------------------------------------------------------ 
//servo variables and timing control
//-------------------------------------------------------------

int servoPos = 0;

unsigned long lastServoMove = 0;
const int servoInterval = 20; // ms between servo moves

//------------------------------------------------------------
// state machine definitions 
//------------------------------------------------------------

enum SystemState {
  INIT,
  MONITORING,
  SERVO_PRESSING,
  MANUAL
};

enum FanState {
  FAN_OFF,
  FAN_ON
};

enum FanAction {
  ACTION_NONE,
  ACTION_TURN_ON,
  ACTION_TURN_OFF
};

enum ServoPressState {
  PRESS_IDLE,
  MOVING_TO_PRESS,
  HOLDING_PRESS,
  RETURNING_TO_NEUTRAL
};

//------------------------------------------------------------
//global system state variables
//------------------------------------------------------------

SystemState currentState = INIT;
SystemState lastPrintedState = MANUAL;

FanState fanState = FAN_OFF;
FanAction pendingAction = ACTION_NONE;
ServoPressState servoPressState = PRESS_IDLE;

//------------------------------------------------------------
// timing variables
//------------------------------------------------------------

unsigned long lastStateChange = 0;
unsigned long stateInterval = 2000;

unsigned long lastTempCheck = 0;
const unsigned long tempCheckInterval = 2000;

unsigned long pressHoldStart = 0;
const unsigned long pressHoldTime = 500;

//------------------------------------------------------------
// servo positions
// ------------------------------------------------------------

const int servoNeutralPos = 0;
const int servoPressPos = 40;

//------------------------------------------------------------
// temp control / hysteresis, fan turns on above 80F and turns off below 76F
//------------------------------------------------------------

// temp thresholds
const float fanOnTempF = 80.0;
const float fanOffTempF = 76.0;

//------------------sensor data storage-----------------------

float currentTempF = 0.0;
float humidity = 0.0;

//------------------------------------------------------------
//serial port printing for debugging
//-------------------------------------------------------------

void printState(SystemState state) {
  switch (state) {
    case INIT: Serial.println("STATE: INIT"); break;
    case MONITORING: Serial.println("STATE: MONITORING"); break;
    case SERVO_PRESSING: Serial.println("STATE: SERVO_PRESSING"); break;
    case MANUAL: Serial.println("STATE: MANUAL"); break;
  }
}

void printStateIfChanged() {
  if (currentState != lastPrintedState) {
    printState(currentState);
    lastPrintedState = currentState;
  }
}

void printFanState() {
  switch (fanState) {
    case FAN_OFF: Serial.println("FAN STATE: OFF"); break;
    case FAN_ON: Serial.println("FAN STATE: ON"); break;
  }
}

//------------------------------------------------------------
// servo control, button press logic
//------------------------------------------------------------

void startServoPress() {
  servoPressState = MOVING_TO_PRESS;
  lastServoMove = millis();
}

bool updateServoPress(unsigned long now) {
  switch (servoPressState) {
    case PRESS_IDLE:
      return true;

    case MOVING_TO_PRESS:
      if (now - lastServoMove >= servoInterval) {
        lastServoMove = now;

        if (servoPos < servoPressPos) {
          servoPos++;
          myServo.write(servoPos);
        } else {
          servoPressState = HOLDING_PRESS;
          pressHoldStart = now;
        }
      }
      break;

    case HOLDING_PRESS:
      if (now - pressHoldStart >= pressHoldTime) {
        servoPressState = RETURNING_TO_NEUTRAL;
        lastServoMove = now;
      }
      break;

    case RETURNING_TO_NEUTRAL:
      if (now - lastServoMove >= servoInterval) {
        lastServoMove = now;

        if (servoPos > servoNeutralPos) {
          servoPos--;
          myServo.write(servoPos);
        } else {
          servoPressState = PRESS_IDLE;
          return true;
        }
      }
      break;
  }

  return false;
}

//------------------------------------------------------------
// temp decision logic
//------------------------------------------------------------

void checkTemperatureLogic() {
  if (fanState == FAN_OFF && currentTempF >= fanOnTempF) {
    pendingAction = ACTION_TURN_ON;
    currentState = SERVO_PRESSING;
    lastStateChange = millis();
    startServoPress();
    Serial.println("Temp high -> request FAN ON");
  }
  else if (fanState == FAN_ON && currentTempF <= fanOffTempF) {
    pendingAction = ACTION_TURN_OFF;
    currentState = SERVO_PRESSING;
    lastStateChange = millis();
    startServoPress();
    Serial.println("Temp low -> request FAN OFF");
  }
}
//------------------------------------------------------------
// BBB serial 
//------------------------------------------------------------

void sendDataToBBB() {
  BBBSerial.print("TEMP_F=");
  BBBSerial.print(currentTempF);
  BBBSerial.print(",HUM=");
  BBBSerial.print(humidity);
  BBBSerial.print(",FAN=");
  if (fanState == FAN_ON) {
    BBBSerial.print("ON");
  } else {
    BBBSerial.print("OFF");
  }
  BBBSerial.print(",STATE=");
  switch (currentState) {
    case INIT: BBBSerial.println("INIT"); break;
    case MONITORING: BBBSerial.println("MONITORING"); break;
    case SERVO_PRESSING: BBBSerial.println("SERVO_PRESSING"); break;
    case MANUAL: BBBSerial.println("MANUAL"); break;
  }
}

//------------------------------------------------------------
// setup and main loop
//------------------------------------------------------------

void setup() {
  Serial.begin(115200);
  delay(500);

  currentState = INIT;
  lastPrintedState = MANUAL;   // force first print
  lastStateChange = millis();

  myServo.attach(18); // gpio pin 18 on esp32
  myServo.write(servoNeutralPos);
  servoPos = servoNeutralPos;

  dht.begin();

  printFanState();

  BBBSerial.begin(115200, SERIAL_8N1, 16, 17);  // RX, TX for BBB  

}

void loop() {
  unsigned long now = millis();

  printStateIfChanged();

//--------------initial state------------------------------

  switch (currentState) {
    case INIT:
      if (now - lastStateChange > stateInterval) {
        currentState = MONITORING;
        lastStateChange = now;
      }
      break;

//------------monitoring state--------------------------

    case MONITORING:
      if (now - lastTempCheck >= tempCheckInterval) {
        lastTempCheck = now;

        float tempC = dht.readTemperature();
        float hum = dht.readHumidity();

        if (!isnan(tempC) && !isnan(hum)) {
          currentTempF = tempC * 9.0 / 5.0 + 32.0;
          humidity = hum;

          Serial.print("Temp F: ");
          Serial.print(currentTempF);
          Serial.print(" | Humidity: ");
          Serial.println(humidity); 

          sendDataToBBB();

          checkTemperatureLogic();
        } else {
          Serial.println("DHT read failed");
        }
      }
      break;

//--------------------servo action state-----------------------

    case SERVO_PRESSING:
      if (updateServoPress(now)) {
        if (pendingAction == ACTION_TURN_ON) {
          fanState = FAN_ON;
          Serial.println("Fan toggled ON");
        }
        else if (pendingAction == ACTION_TURN_OFF) {
          fanState = FAN_OFF;
          Serial.println("Fan toggled OFF");
        }

        pendingAction = ACTION_NONE;
        printFanState();

        currentState = MONITORING;
        lastStateChange = now;
      }
      break;

//----------------------manual state (not used rn)-----------------------
    case MANUAL:
      break;
  }

//--------------------------testing thingy for bbb serial--------------------
// static unsigned long lastSend = 0;

// if (millis() - lastSend >= 1000) {
//   lastSend = millis();
//   BBBSerial.println("HELLO_FROM_ESP32");
// }
//--------------------------end of testing thingy-----------------------------


}