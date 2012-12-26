#!/bin/bash -ex

scons
scons test

mkdir -p isobuild/boot
cat build/x86-pc/source/platform/pc/stage1/cdboot build/x86-pc/source/loader > isobuild/boot/cdboot.img
cp build/x86-pc/test/test32.elf build/x86-pc/test/test64.elf isobuild/

cat > isobuild/boot/loader.cfg << EOF
set "timeout" 5

entry "Test (32-bit)" {
	kboot "/test32.elf" ["/test32.elf"]
}

entry "Test (64-bit)" {
	kboot "/test64.elf" ["/test64.elf"]
}

entry "Chainload (hd0)" {
	device "(hd0)"
	chainload
}
EOF

mkisofs -J -R -l -b boot/cdboot.img -V "CDROM" -boot-load-size 4 -boot-info-table -no-emul-boot -o build/x86-pc/test.iso isobuild
rm -rf isobuild
qemu-system-x86_64 -cdrom build/x86-pc/test.iso -serial stdio -vga std -boot d -m 512
