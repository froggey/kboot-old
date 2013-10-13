#!/bin/bash -ex

scons
#scons test

mkdir -p bimgbuild/boot
#cp build/mips-malta/test/test.elf bimgbuild/

cat > bimgbuild/boot/loader.cfg << EOF
kboot "/test.elf" []
EOF

cd bimgbuild
tar -cvf ../build/mips-malta/boot.tar *
cd ..
rm -rf bimgbuild

qemu-system-mipsel -M malta -cpu 74Kf -m 256 -monitor vc:1024x768 -serial stdio \
	-kernel build/mips-malta/source/loader.elf \
	-initrd build/mips-malta/boot.tar
