#!/usr/bin/env python
# -*- coding: utf-8 -*-

import hal
import sys
import signal
import linuxcnc
import time

from serial import Serial, SerialException


class MpgDro:

    def __init__(self):

        signal.signal(signal.SIGTERM, self.signal_handler)
        signal.signal(signal.SIGINT, self.signal_handler)

        self.ser = Serial()
        self.ser.baudrate = 1000000
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
        self.interval = 50

        self.c = linuxcnc.command()
        self.s = linuxcnc.stat()

        self.position = [0.0, 0.0, 0.0]
        self.actual = [0.0, 0.0, 0.0]
        self.offset = [0.0, 0.0, 0.0]
        self.velocity = [0.0, 0.0, 0.0]

        self.feed = 100

    def main(self):

        while self.running:

            self.read_data()
            # self.send_data()

        self.ser.close()

    def signal_handler(self, signal, frame):

        self.running = False

    def read_data(self):

        while self.ser.inWaiting():

            in_data = self.ser.readline()

            msg_type = in_data[0:4]
            msg = in_data[4::]

            if msg_type == "FEED":
                print("FEED")

            elif msg_type == "STEP":
                # print("STEP")

                # print(msg)

                axis_val = 0
                step_val = 0
                step_dir_val = 0

                step_data = msg[0]
                axis_data = msg[1]
                dir_data = msg[2]

                if step_data == '1':
                    if dir_data == '1':
                        step_val = 1.0
                    elif dir_data == '0':
                        step_val = -1.0
                elif step_data == '0':
                    if dir_data == '1':
                        step_val = 0.5
                    elif dir_data == '0':
                        step_val = -0.5
                elif step_data == '2':
                    if dir_data == '1':
                        step_val = 0.1
                    elif dir_data == '0':
                        step_val = -0.1

                if axis_data == '1':
                    axis_val = 0
                elif axis_data == '0':
                    axis_val = 1
                elif axis_data == '2':
                    axis_val = 2

                # print(axis_val, step_val)

                self.c.jog(linuxcnc.JOG_INCREMENT, False, axis_val, 100, step_val)

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


def main():
    h = hal.component("mpg_dro")
    h.ready()
    mpg_dro = MpgDro()
    mpg_dro.main()


if __name__ == "__main__":
    main()
