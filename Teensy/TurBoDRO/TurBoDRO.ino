#include <ArduinoJson.h>

#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Encoder.h>
#include <LedControl.h>
#include <FastCRC.h>

FastCRC8 CRC8;

Encoder knob(0, 1);

LedControl lc = LedControl(12, 11, 10, 3);

StaticJsonBuffer<200> feedJsonBuffer;
StaticJsonBuffer<200> stepJsonBuffer;
StaticJsonBuffer<500> droJsonBuffer;

JsonObject& feedRoot = feedJsonBuffer.createObject();
JsonObject& stepRoot = stepJsonBuffer.createObject();

JsonObject& stepData = stepRoot.createNestedObject("step");


// Pot Stuff
unsigned long potPreviousMillis = 0;
unsigned long potInterval = 10;

const int num_readings = 10;

int readings[num_readings];      // the readings from the analog input
int read_index = 0;              // the index of the current reading
int total = 0;                  // the running total
int average = 0;                // the average

int pot_pin = A0;

int prev_feed = 0;
int feed = 0;

// Encoder

int steps = 0;
long knob_position  = 0;

// Switch

int step_1 = 20;
int step_2 = 21;

int axis_1 = 22;
int axis_2 = 23;

boolean jsonReceived = false;

int x_velocity = 0;
int y_velocity = 0;
int z_velocity = 0;

int x_leds = 0;
int y_leds = 0;
int z_leds = 0;

byte bar[11][2] = {
  { B01100000, B01110000 }, //  1111100000
  { B00100000, B01110000 }, //  0111100000
  { B00000000, B01110000 }, //  0011100000
  { B00000000, B00110000 }, //  0001100000
  { B00000000, B00010000 }, //  0000100000
  { B00000000, B00000000 }, //  0000000000
  { B00000000, B00001000 }, //  0000010000
  { B00000000, B00001100 }, //  0000011000
  { B00000000, B00001110 }, //  0000011100
  { B00000000, B00001111 }, //  0000011110
  { B00000000, B10001111 }, //  0000011111
};

void readKnob();
void getData();
void draw();

void setup() {

  feedRoot["feed"] = 0;

  stepRoot["step"];


  // initialize all the readings to 0:
  for (int this_reading = 0; this_reading < num_readings; this_reading++) {
    readings[this_reading] = 0;
  }

  pinMode(axis_1, INPUT);
  pinMode(axis_2, INPUT);
  pinMode(step_1, INPUT);
  pinMode(step_2, INPUT);

  // initialize serial:
  Serial.begin(57600);

  int devices = lc.getDeviceCount();

  for (int address = 0; address < devices; address++) {  // we have to init all devices in a loop
    lc.shutdown(address, false);  /* The MAX72XX is in power-saving mode on startup*/
    lc.setIntensity(address, 8);  /* Set the brightness to a medium values */
    lc.clearDisplay(address);     /* and clear the display */
  }

  lc.setChar(0, 0, ' ', false);
  lc.setChar(0, 1, '0', false);
  lc.setChar(0, 2, '0', false);
  lc.setChar(0, 3, '0', true);
  lc.setChar(0, 4, '0', false);
  lc.setChar(0, 5, '0', false);

  lc.setChar(1, 0, ' ', false);
  lc.setChar(1, 1, '0', false);
  lc.setChar(1, 2, '0', false);
  lc.setChar(1, 3, '0', true);
  lc.setChar(1, 4, '0', false);
  lc.setChar(1, 5, '0', false);

  lc.setChar(2, 0, ' ', false);
  lc.setChar(2, 1, '0', false);
  lc.setChar(2, 2, '0', false);
  lc.setChar(2, 3, '0', true);
  lc.setChar(2, 4, '0', false);
  lc.setChar(2, 5, '0', false);

}

void loop() {
  readPot();
  readKnob();
  getSerialData();
}


void draw(JsonObject& droRoot) {

  int x_velocity = droRoot["DRO"]["X"]["vel"];
  int y_velocity = droRoot["DRO"]["Y"]["vel"];
  int z_velocity = droRoot["DRO"]["Z"]["vel"];

  x_leds = map(x_velocity, -2500, 2500, 0, 10);
  y_leds = map(y_velocity, -2500, 2500, 0, 10);
  z_leds = map(z_velocity, -2500, 2500, 0, 10);

  lc.setRow(2, 7, bar[x_leds][0]);
  lc.setRow(2, 6, bar[x_leds][1]);

  lc.setRow(1, 7, bar[y_leds][0]);
  lc.setRow(1, 6, bar[y_leds][1]);

  lc.setRow(0, 7, bar[z_leds][0]);
  lc.setRow(0, 6, bar[z_leds][1]);


  String x_axis = droRoot["DRO"]["X"]["pos"];
  String y_axis = droRoot["DRO"]["Y"]["pos"];
  String z_axis = droRoot["DRO"]["Z"]["pos"];

  lc.setChar(2, 0, x_axis[0], false);
  lc.setChar(2, 1, x_axis[1], false);
  lc.setChar(2, 2, x_axis[2], false);
  lc.setChar(2, 3, x_axis[3], true);
  lc.setChar(2, 4, x_axis[4], false);
  lc.setChar(2, 5, x_axis[5], false);

  lc.setChar(1, 0, y_axis[0], false);
  lc.setChar(1, 1, y_axis[1], false);
  lc.setChar(1, 2, y_axis[2], false);
  lc.setChar(1, 3, y_axis[3], true);
  lc.setChar(1, 4, y_axis[4], false);
  lc.setChar(1, 5, y_axis[5], false);

  lc.setChar(0, 0, z_axis[0], false);
  lc.setChar(0, 1, z_axis[1], false);
  lc.setChar(0, 2, z_axis[2], false);
  lc.setChar(0, 3, z_axis[3], true);
  lc.setChar(0, 4, z_axis[4], false);
  lc.setChar(0, 5, z_axis[5], false);
}


void readKnob() {

  int axis_val = 0;
  int step_val = 0;

  bitWrite(axis_val, 0, !digitalRead(axis_1));
  bitWrite(axis_val, 1, !digitalRead(axis_2));

  bitWrite(step_val, 0, !digitalRead(step_1));
  bitWrite(step_val, 1, !digitalRead(step_2));

  long new_pos;

  new_pos = knob.read();

  if (new_pos != knob_position) {
    steps += 1;
    if (steps == 4) {

      stepData["axis"] = axis_val;
      stepData["dist"] = step_val;

      if (new_pos > knob_position) {
        stepData["dir"] = 0;
      }
      else {
        stepData["dir"] = 1;
      }

      Serial.write(0x02);
      stepRoot.printTo(Serial);
      // Serial.print(CRC8.smbus(output, sizeof(output)), HEX );
      Serial.write(0x03);

      steps = 0;
    }
    knob_position = new_pos;
  }
}

void readPot() {

  unsigned long currentMillis = millis();

  if (currentMillis - potPreviousMillis > potInterval) {
    // save the last time you blinked the LED
    potPreviousMillis = currentMillis;



    // subtract the last reading:
    total = total - readings[read_index];


    // read from the sensor:
    readings[read_index] = map(analogRead(pot_pin), 0, 1024, 0, 121);


    // add the reading to the total:
    total = total + readings[read_index];

    // advance to the next position in the array:
    read_index = read_index + 1;

    // if we're at the end of the array...
    if (read_index >= num_readings) {
      // ...wrap around to the beginning:
      read_index = 0;
    }

    // calculate the average:
    feed = total / num_readings;

    if (prev_feed != feed) {
      prev_feed = feed;

      feedRoot["feed"] = feed;

      Serial.write(0x02);
      feedRoot.printTo(Serial);
      // Serial.print(CRC8.smbus(output, sizeof(output)), HEX );
      Serial.write(0x03);
    }
  }
}

void getSerialData() {
  while (Serial.available()) {
    JsonObject& droRoot = droJsonBuffer.parseObject(Serial);
    if (droRoot.success()) {
      //droRoot.printTo(Serial);
      draw(droRoot);
      droJsonBuffer.clear();
    }
  }
}

