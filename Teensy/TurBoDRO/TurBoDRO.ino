#define ENCODER_OPTIMIZE_INTERRUPTS

#include <Encoder.h>
#include <LedControl.h>

Encoder knob(0, 1);

LedControl lc = LedControl(12, 11, 10, 3);


// Pot Stuff
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

int axis_1 = 20;
int axis_2 = 21;

int step_1 = 22;
int step_2 = 23;

String inputString = "";         // a string to hold incoming data
boolean stringComplete = false;  // whether the string is complete

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

  // initialize all the readings to 0:
  for (int this_reading = 0; this_reading < num_readings; this_reading++) {
    readings[this_reading] = 0;
  }

  pinMode(axis_1, INPUT);
  pinMode(axis_2, INPUT);
  pinMode(step_1, INPUT);
  pinMode(step_2, INPUT);

  // initialize serial:
  Serial.begin(1000000);

  // reserve 200 bytes for the inputString:
  inputString.reserve(200);

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
  
  if (stringComplete) {
    draw();
    inputString = "";
    stringComplete = false;
  }
}


void draw() {
  // print the string when a newline arrives:

  x_velocity = inputString.substring(18, 24).toInt();
  y_velocity = inputString.substring(24, 30).toInt();
  z_velocity = inputString.substring(30, 36).toInt();

  x_leds = map(x_velocity, -4500, 4500, 0, 11);
  y_leds = map(y_velocity, -4500, 4500, 0, 11);
  z_leds = map(z_velocity, -4500, 4500, 0, 11);

  lc.setRow(0, 7, bar[x_leds][0]);
  lc.setRow(0, 6, bar[x_leds][1]);

  lc.setRow(1, 7, bar[y_leds][0]);
  lc.setRow(1, 6, bar[y_leds][1]);

  lc.setRow(2, 7, bar[z_leds][0]);
  lc.setRow(2, 6, bar[z_leds][1]);

  lc.setChar(0, 0, inputString[0], false);
  lc.setChar(0, 1, inputString[1], false);
  lc.setChar(0, 2, inputString[2], false);
  lc.setChar(0, 3, inputString[3], true);
  lc.setChar(0, 4, inputString[4], false);
  lc.setChar(0, 5, inputString[5], false);

  lc.setChar(1, 0, inputString[6], false);
  lc.setChar(1, 1, inputString[7], false);
  lc.setChar(1, 2, inputString[8], false);
  lc.setChar(1, 3, inputString[9], true);
  lc.setChar(1, 4, inputString[10], false);
  lc.setChar(1, 5, inputString[11], false);

  lc.setChar(2, 0, inputString[12], false);
  lc.setChar(2, 1, inputString[13], false);
  lc.setChar(2, 2, inputString[14], false);
  lc.setChar(2, 3, inputString[15], true);
  lc.setChar(2, 4, inputString[16], false);
  lc.setChar(2, 5, inputString[17], false);

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
    // Serial.print("Steps = ");
    // Serial.println(steps);
    if (steps == 4) {
      Serial.print("STEP");
      Serial.print(axis_val);
      Serial.print(step_val);
      if (new_pos > knob_position) {
        Serial.println(0);
      }
      else {
        Serial.println(1);
      }
      steps = 0;
    }
    knob_position = new_pos;
  }
}

void readPot() {
  
  // subtract the last reading:
  total = total - readings[read_index];

  
  // read from the sensor:
  readings[read_index] = map(analogRead(pot_pin), 0, 1023, 0, 120);

  
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
    // send it to the computer as ASCII digits
    Serial.print("FEED");
    Serial.println(feed);
    //delay(1);        // delay in between reads for stability
  }
  
}

void getSerialData() {
  while (Serial.available()) {
    char inChar = (char)Serial.read();
    if (inChar == '\n') {
      stringComplete = true;
    }
    else {
      inputString += inChar;
    }
  }
}
