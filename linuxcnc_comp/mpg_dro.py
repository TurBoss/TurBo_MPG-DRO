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
        self.ser.baudrate = 57600
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
            self.read_data()
            self.send_data()

        self.ser.close()

    def signal_handler(self, signal, frame):

        self.running = False

    def read_data(self):

        while self.ser.inWaiting():

            byte_list = list()

            in_byte = self.ser.read()

            if in_byte == '\x02':
                while in_byte != '\x03':
                    in_byte = self.ser.read()
                    if in_byte != '\x03':
                        byte_list.append(in_byte)

                json_string = ''.join(byte_list)
                in_data = json.loads(json_string)

                for k, v in in_data.items():
                    if k == "feed":
                        self.c.feedrate(v / 100)
                    elif k == "step":

                        if not self.s.estop and \
                                self.s.enabled and \
                                self.s.homed[0] and \
                                self.s.homed[1] and \
                                self.s.homed[2]:

                            axis_val = 0
                            dir_val = 0

                            dist_data = v["dist"]
                            axis_data = v["axis"]
                            dir_data = v["dir"]

                            if dist_data == 1:
                                if dir_data == 1:
                                    dir_val = 1.0
                                elif dir_data == 0:
                                    dir_val = -1.0

                            elif dist_data == 0:
                                if dir_data == 1:
                                    dir_val = 0.5
                                elif dir_data == 0:
                                    dir_val = -0.5

                            elif dist_data == 2:
                                if dir_data == 1:
                                    dir_val = 0.1
                                elif dir_data == 0:
                                    dir_val = -0.1

                            if axis_data == 1:
                                axis_val = 0
                            elif axis_data == 0:
                                axis_val = 1
                            elif axis_data == 2:
                                axis_val = 2

                            self.c.jog(linuxcnc.JOG_INCREMENT, False, axis_val, 100, dir_val)

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


            """
            data = "{0:+07.2f}{1:+07.2f}{2:+07.2f}{3:+07.2f}{4:+07.2f}{5:+07.2f}\n" \
                .format(self.position[2],
                        self.position[1],
                        self.position[0],
                        self.velocity[2],
                        self.velocity[1],
                        self.velocity[0]) \
                .replace(".", "") \
                .replace("+", " ")
            self.ser.write(data)
            
            """

            self.data = {
                "DRO": {
                    "X": {
                        "pos": "{0:+07.2f}".format(self.position[0]),
                        "vel": "{0:+07.2f}".format(self.velocity[0])
                    },
                    "Y": {
                        "pos": "{0:+07.2f}".format(self.position[1]),
                        "vel": "{0:+07.2f}".format(self.velocity[1])
                    },
                    "Z": {
                        "pos": "{0:+07.2f}".format(self.position[2]),
                        "vel": "{0:+07.2f}".format(self.velocity[2])
                    }
                }
            }

            update = False

            for x in ["X", "Y", "Z"]:
                if self.data.get("DRO").get(x).get("pos") != self.previous_data.get("DRO").get(x).get("pos"):
                    self.previous_data = copy(self.data)
                    update = True

            if update:
                self.ser.write(self.data)


def main():
    h = hal.component("mpg_dro")
    h.ready()
    mpg_dro = MpgDro()
    mpg_dro.main()


if __name__ == "__main__":
    main()
