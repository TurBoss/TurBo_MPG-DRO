#include <ArduinoJson.h>

#include <LedControl.h>
#include <FastCRC.h>

FastCRC8 CRC8;

LedControl lc = LedControl(12, 11, 10, 3);

DynamicJsonDocument dro(512);

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

void getSerialData();
void draw();

void setup() {

  // initialize serial:
  Serial.begin(115200);

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
}

void draw(DynamicJsonDocument& dro) {

  int x_velocity = dro["DRO"]["X"]["vel"];
  int y_velocity = dro["DRO"]["Y"]["vel"];
  int z_velocity = dro["DRO"]["Z"]["vel"];

  x_leds = map(x_velocity, -2500, 2500, 0, 10);
  y_leds = map(y_velocity, -2500, 2500, 0, 10);
  z_leds = map(z_velocity, -2500, 2500, 0, 10);

  lc.setRow(2, 7, bar[x_leds][0]);
  lc.setRow(2, 6, bar[x_leds][1]);

  lc.setRow(1, 7, bar[y_leds][0]);
  lc.setRow(1, 6, bar[y_leds][1]);

  lc.setRow(0, 7, bar[z_leds][0]);
  lc.setRow(0, 6, bar[z_leds][1]);


  String x_axis = dro["DRO"]["X"]["pos"];
  String y_axis = dro["DRO"]["Y"]["pos"];
  String z_axis = dro["DRO"]["Z"]["pos"];

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

void serialEvent() {
  while (Serial.available()) {
    DeserializationError error = deserializeJson(dro, Serial);
    if (error) {
      return;
    }
    //droRoot.printTo(Serial);
    draw(dro);
    dro.clear();
  }
}
