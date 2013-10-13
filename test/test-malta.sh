#!/bin/bash -ex

scons
qemu-system-mipsel -M malta -cpu 74Kf -kernel build/mips-malta/source/loader.elf -m 256 -monitor vc:1024x768 -serial stdio