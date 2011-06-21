#!/bin/bash -ex

scons
scons test

mkdir -p isobuild/boot
cat build/x86-pc/src/platform/pc/cdboot build/x86-pc/src/loader > isobuild/boot/cdboot.img
cp build/x86-pc/test/test.elf isobuild/

cat > isobuild/boot/loader.cfg << EOF
set "timeout" 5

entry "Test" {
	kboot "/test.elf" []
}
EOF

mkisofs -J -R -l -b boot/cdboot.img -V "CDROM" -boot-load-size 4 -boot-info-table -no-emul-boot -o build/x86-pc/test.iso isobuild
rm -rf isobuild
qemu -cdrom build/x86-pc/test.iso -serial stdio -vga std -boot d -m 512
