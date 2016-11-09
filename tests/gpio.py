#!/usr/bin/env python3

import unittest

GPIO_BASE = 480
GPIO_COUNT = 32

class Gpio(object):
    SYSFS_ROOT = '/sys/class/gpio/'
    SYSFS_EXPORT = '/sys/class/gpio/export'
    SYSFS_UNEXPORT = '/sys/class/gpio/unexport'
    OUT = 'out'
    IN = 'in'
    HIGH = '1'
    LOW = '0'

    def __init__(self, gpio_base, gpio):
        self.gpio = str(gpio_base + gpio)
        sysfs_gpio = self.SYSFS_ROOT + 'gpio' + self.gpio + '/'
        self.sysfs_direction = sysfs_gpio + 'direction'
        self.sysfs_value = sysfs_gpio + 'value'

    def __enter__(self):
        self.export()
        return self

    def __exit__(self, type, value, traceback):
        self.unexport()

    def export(self):
        with open(self.SYSFS_EXPORT, 'w') as f:
            f.write(self.gpio)

    def unexport(self):
        with open(self.SYSFS_UNEXPORT, 'w') as f:
            f.write(self.gpio)

    def set_direction(self, direction):
        with open(self.sysfs_direction, 'w') as f:
            f.write(direction)

    def set_output(self):
        self.set_direction(self.OUT)

    def set_input(self):
        self.set_direction(self.IN)

    def get_direction(self):
        with open(self.sysfs_direction, 'r') as f:
            return f.read().rstrip()

    def write(self, value):
        with open(self.sysfs_value, 'w') as f:
            f.write(value)

    def set_high(self):
        self.write(self.HIGH)

    def set_low(self):
       self.write(self.LOW)

    def get_value(self):
        with open(self.sysfs_value, 'r') as f:
            return f.read().rstrip()

class TestGpio(unittest.TestCase):

    def test_configure_as_output(self):
        for i in range(0, GPIO_COUNT - 1):
            with Gpio(GPIO_BASE, i) as g:
                g.set_input()
                g.set_output()
                self.assertEqual(g.get_direction(), Gpio.OUT)

    def test_configure_as_input(self):
        for i in range(0, GPIO_COUNT - 1):
            with Gpio(GPIO_BASE, i) as g:
                g.set_output()
                g.set_input()
                self.assertEqual(g.get_direction(), Gpio.IN)

    def test_set_low(self):
        for i in range(0, GPIO_COUNT - 1):
            with Gpio(GPIO_BASE, i) as g:
                g.set_high()
                self.assertEqual(g.get_value(), Gpio.HIGH)
                g.set_low()
                self.assertEqual(g.get_value(), Gpio.LOW)

    def test_set_high(self):
        for i in range(0, GPIO_COUNT - 1):
            with Gpio(GPIO_BASE, i) as g:
                g.set_low()
                self.assertEqual(g.get_value(), Gpio.LOW)
                g.set_high()
                self.assertEqual(g.get_value(), Gpio.HIGH)

if __name__ == '__main__':
    unittest.main()
