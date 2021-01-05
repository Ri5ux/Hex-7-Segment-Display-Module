/** Hex Segment Display Controller (1-4-2021) **/

#include <RF24.h>
#include <printf.h> 
#include <SoftwareSerial.h>

static const byte NUM_MATRIX[16][4] = { 
  { 0,0,0,0 }, //0
  { 0,0,0,1 }, //1
  { 0,0,1,0 }, //2
  { 0,0,1,1 }, //3
  { 0,1,0,0 }, //4
  { 0,1,0,1 }, //5
  { 0,1,1,0 }, //6
  { 0,1,1,1 }, //7
  { 1,0,0,0 }, //8
  { 1,0,0,1 }, //9
  { 1,0,1,0 }, //10 (c)
  { 1,0,1,1 }, //11 (c - reversed)
  { 1,1,0,0 }, //12 (u - top)
  { 1,1,0,1 }, //13 (c -top w/ underline)
  { 1,1,1,0 }, //14 (t)
  { 1,1,1,1 }  //15 (none)
};
static const uint8_t DISPLAYS = 3;
static const uint8_t DRIVERS = 2;
static const uint8_t DRIVER_INPUTS = 4;
static const uint8_t PINS_DISP_EN[DISPLAYS] = { 6, A0, A5 };
static const uint8_t PINS_DRIVER[DRIVERS][DRIVER_INPUTS] = {
  { A1, A2, A3, A4 },
  { 02, 03, 04, 05 }
};
static uint8_t displayBuffer[DRIVERS][DISPLAYS] = {
  { 15, 15, 15 },
  { 15, 15, 15 }
};

RF24 radio(7, 8);
const byte addresses[][6] = {"10002", "10001"};

const int PIN_BT_RX = 9;
const int PIN_BT_TX = 10;

/** Bluetooth (HXC-05) **/
SoftwareSerial bluetooth(PIN_BT_RX, PIN_BT_TX); //RX, TX
boolean streamData = false;
long previousMillis = 0;

float scaleValue = 0;

void(* resetFunc) (void) = 0;

void setup() {
  Serial.begin(9600);
  printf_begin();
  Serial.println("Hex Segment Display Controller");
  Serial.println("ASX Electronics");
  Serial.println("");

  Serial.println("[init] radio");
  radio.begin();
  radio.openWritingPipe(addresses[1]);
  radio.openReadingPipe(1, addresses[0]);
  radio.setPALevel(RF24_PA_LOW);
  radio.printDetails();
  
  Serial.println("[bluetooth] init");
  bluetooth.begin(9600);
  bluetooth.println("[bluetooth] ready");

  Serial.println("[init] display");
  
  for (uint8_t i = 0; i < DRIVERS; i++) {
    for (uint8_t i1 = 0; i1 < DRIVER_INPUTS; i1++) {
      uint8_t pin = (uint8_t)PINS_DRIVER[i][i1];
      pinMode(pin, OUTPUT);
    }
  }
  
  for (uint8_t i = 0; i < DISPLAYS; i++) {
      uint8_t pin = (uint8_t)PINS_DISP_EN[i];
      pinMode(pin, OUTPUT);
  }
  
  Serial.println("[init] done");
}

void loop() {
  handleBluetoothCommands();
  handleRadio();
  
  if (Serial.available() > 0) {
   char incomingCharacter = Serial.read();
   switch (incomingCharacter) {
     case '1':
      displayBuffer[0][0] = (uint8_t)Serial.parseInt();
      break;
     case '2':
      displayBuffer[0][1] = (uint8_t)Serial.parseInt();
      break;
     case '3':
      displayBuffer[0][2] = (uint8_t)Serial.parseInt();
      break;
     case '4':
      displayBuffer[1][0] = (uint8_t)Serial.parseInt();
      break;
     case '5':
      displayBuffer[1][1] = (uint8_t)Serial.parseInt();
      break;
     case '6':
      displayBuffer[1][2] = (uint8_t)Serial.parseInt();
      break;
     case 'r':
      Serial.println("reset");
      resetFunc();
      break;
     case 't':
      Serial.println("transmit");
      pingRF();
      break;
    }
  }

  driveDisplay();
  handleBTData();
}

void handleRadio() {
  
  delay(1);
  radio.startListening();
  
  if (radio.available()) {
    float values[7] = {0};
    radio.read(&values, sizeof(values));
    displayBuffer[0][0] = values[0];
    displayBuffer[0][1] = values[1];
    displayBuffer[0][2] = values[2];
    displayBuffer[1][0] = values[3];
    displayBuffer[1][1] = values[4];
    displayBuffer[1][2] = values[5];
    scaleValue = values[6];
  }

  delay(1);
  radio.stopListening();
}

void handleBTData() {
  long currentMillis = millis();
  
  if(currentMillis - previousMillis > 500) {
    previousMillis = currentMillis;
    
    if (streamData) {
      bluetooth.print("value: ");
      bluetooth.println(scaleValue);
    }
  }
}

void handleBluetoothCommands() {
  if (bluetooth.available()) {
    String command = bluetooth.readStringUntil('\n');
    bluetooth.read();
    bluetooth.print(">");
    bluetooth.println(command);
    
    if (command == "reset") {
      bluetooth.println("reset");
      delay(100);
      resetFunc();
    } else if (command == "read") {
      bluetooth.print("read: ");
      bluetooth.println(scaleValue);
    } else if (command == "stream") {
      streamData = true;
    }
  }
}

void driveDisplay() {
  for (uint8_t id = 0; id < DISPLAYS; id++) {
    blankAll();
    writeIntToDisplayDriver(displayBuffer[0][id], PINS_DRIVER[0]);
    writeIntToDisplayDriver(displayBuffer[1][id], PINS_DRIVER[1]);
    digitalWrite(PINS_DISP_EN[id], HIGH);
    delay(1);
  }
  
  blankAll();
}

void blankAll() {
  for (uint8_t id = 0; id < DISPLAYS; id++) {
    digitalWrite(PINS_DISP_EN[id], LOW);
  }
}

void writeIntToDisplayDriver(uint8_t num, uint8_t driverPins[]) {
  uint8_t p = 3;
  for (uint8_t i = 0; i < 4; i++) {
    digitalWrite(driverPins[p], NUM_MATRIX[num][i]);
    p--;
  }
}

void pingRF() {
  const char text[] = "ping";
  bool result = radio.write(&text, sizeof(text));
}
