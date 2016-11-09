## SUB-20 Kernel Module

I've developed this for kernel version 4.4. I'll try my best to support other
kernel versions if requested.

### Installation

1. Install GCC and the headers for your running kernel.
  1. On Ubuntu 16.04: `sudo apt install gcc linux-headers-generic`
1. `make && sudo make install`

You can now plug in your SUB-20, you should see some details in `dmesg` and your
sysfs will be populated with relevant sub-devices.

## Done

* GPIO
 * Write
 * Read
 * Configure
 * Unit tests

## To Do

* General
 * Test multple SUB-20s on one USB hub
 * Get serial number
 * Get product ID
 * Get version
 * Reset
* EEPROM
 * Everything
* I2C
 * Everything
* SPI
 * Everything
* MDIO
 * Everything
* ADC
 * Everything
* LCD
 * Everything
* UART
 * Everything
* GPIO
 * Device tree
 * Set multiple
 * Edge detection
 * Handle other functions claiming GPIOs
* GPIO B
 * Everything
