#include <ArduinoJson.h>

#include <LedControl.h>
#include <FastCRC.h>

LedControl lc = LedControl(12, 11, 10, 3);

DynamicJsonDocument dro(512);

void draw(DynamicJsonDocument& dro);

void setup() {

  // initialize serial:
  Serial.begin(115200);

  int devices = lc.getDeviceCount();

  for (int address = 0; address < devices; address++) {  // we have to init all devices in a loop
    lc.shutdown(address, false);  /* The MAX72XX is in power-saving mode on startup*/
    lc.setIntensity(address, 8);  /* Set the brightness to a medium values */
    lc.clearDisplay(address);     /* and clear the display */
  }

  lc.setChar(0, 0, '0', false);
  lc.setChar(0, 1, '0', false);
  lc.setChar(0, 2, '0', false);
  lc.setChar(0, 3, '0', true);
  lc.setChar(0, 4, '0', false);
  lc.setChar(0, 5, '0', false);

  lc.setChar(1, 0, '0', false);
  lc.setChar(1, 1, '0', false);
  lc.setChar(1, 2, '0', false);
  lc.setChar(1, 3, '0', true);
  lc.setChar(1, 4, '0', false);
  lc.setChar(1, 5, '0', false);

  lc.setChar(2, 0, '0', false);
  lc.setChar(2, 1, '0', false);
  lc.setChar(2, 2, '0', false);
  lc.setChar(2, 3, '0', true);
  lc.setChar(2, 4, '0', false);
  lc.setChar(2, 5, '0', false);
}

void loop() {
  while (Serial.available()) {
    DeserializationError error = deserializeJson(dro, Serial);
    if (error) {
      Serial.println("Error reading JSON");
    }
    else {
      draw(dro);
      dro.clear();
    }
  }
}

void draw(DynamicJsonDocument& dro) {

  String x_axis = dro["DRO"]["X"]["pos"];
  String y_axis  = dro["DRO"]["Y"]["pos"];
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
