#!/usr/bin/env python
# -*- coding: utf-8 -*-

from __future__ import division

import hal
import sys
import signal
import linuxcnc
import time
import json

from serial import Serial, SerialException
from copy import copy


class MpgDro:

    def __init__(self):

        signal.signal(signal.SIGTERM, self.signal_handler)
        signal.signal(signal.SIGINT, self.signal_handler)

        self.ser = Serial()
        self.ser.baudrate = 115200
        self.ser.port = "/dev/ttyACM0"

        try:
            self.ser.open()
            if self.ser.is_open:
                self.running = True
        except SerialException as e:
            print(e)
            sys.exit(1)

        self.current_millis = time.time() * 1000
        self.previous_millis = 0
        self.interval = 10

        self.c = linuxcnc.command()
        self.s = linuxcnc.stat()

        self.position = [0.0, 0.0, 0.0]
        self.actual = [0.0, 0.0, 0.0]
        self.offset = [0.0, 0.0, 0.0]
        self.velocity = [0.0, 0.0, 0.0]

        self.feed = 100

        self.data = dict()
        self.previous_data = {
            "DRO": {
                "X": {
                    "pos": 0,
                    "vel": 0
                },
                "Y": {
                    "pos": 0,
                    "vel": 0
                },
                "Z": {
                    "pos": 0,
                    "vel": 0
                }
            }
        }

    def main(self):

        while self.running:
            self.send_data()

        self.ser.close()

    def signal_handler(self, signal, frame):
        self.running = False

    def send_data(self):

        self.current_millis = time.time() * 1000

        if self.current_millis - self.previous_millis >= self.interval:
            self.previous_millis = self.current_millis

            self.s.poll()  # get current values

            self.actual = self.s.joint_actual_position
            self.offset = self.s.g5x_offset

            for i in range(3):
                self.position[i] = self.actual[i] - self.offset[i]

            self.velocity = [
                self.s.joint[0]["velocity"],
                self.s.joint[1]["velocity"],
                self.s.joint[2]["velocity"]
            ]

            self.data = {
                "DRO": {
                    "X": {
                        "pos": "{0:+07.2f}".format(self.position[0]).replace(".", ""),
                        "vel": "{0:+07.2f}".format(self.velocity[0]).replace(".", "")
                    },
                    "Y": {
                        "pos": "{0:+07.2f}".format(self.position[1]).replace(".", ""),
                        "vel": "{0:+07.2f}".format(self.velocity[1]).replace(".", "")
                    },
                    "Z": {
                        "pos": "{0:+07.2f}".format(self.position[2]).replace(".", ""),
                        "vel": "{0:+07.2f}".format(self.velocity[2]).replace(".", "")
                    }
                }
            }

            update = False

            for x in ["X", "Y", "Z"]:
                if self.data.get("DRO").get(x).get("pos") != self.previous_data.get("DRO").get(x).get("pos"):
                    self.previous_data = copy(self.data)
                    update = True

            if update:
                self.ser.write(json.dumps(self.data))


def main():
    mpg_dro = MpgDro()
    mpg_dro.main()


if __name__ == "__main__":
    main()
