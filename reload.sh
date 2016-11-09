#!/bin/bash

set -e

make
lsmod | grep sub20 && sudo rmmod sub20; sudo insmod sub20.ko
