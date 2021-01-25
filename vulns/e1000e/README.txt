Command line used to find this crash:

/home/cwmyung/prj/hyfuzz/build/afl/afl-fuzz -m none -t 10000 -i /home/cwmyung/prj/hyfuzz/build/afl/tmp_input -o /home/cwmyung/prj/hyfuzz/build/afl/tmp_output -a /home/cwmyung/prj/hyfuzz/build/afl/patterns -s /home/cwmyung/prj/hyfuzz/src/xv6/fs.img -- /home/cwmyung/prj/hyfuzz/src/qemu-5.2/build/i386-softmmu/qemu-system-i386 -device e1000e,netdev=mynet1 -netdev user,id=mynet1,hostfwd=tcp::4040-:22 -device ES1370 -device floppy -device ide-cd -device ich9-usb-ehci1,id=usb -device ich9-usb-uhci1,masterbus=usb.0,firstport=0,multifunction=on -device ich9-usb-uhci2,masterbus=usb.0,firstport=2 -device ich9-usb-uhci3,masterbus=usb.0,firstport=4 -chardev spicevmc,name=usbredir,id=usbredirchardev1 -device usb-redir,chardev=usbredirchardev1,id=usbredirdev1 -chardev spicevmc,name=usbredir,id=usbredirchardev2 -device usb-redir,chardev=usbredirchardev2,id=usbredirdev2 -chardev spicevmc,name=usbredir,id=usbredirchardev3 -device usb-redir,chardev=usbredirchardev3,id=usbredirdev3 -drive file=/home/cwmyung/prj/hyfuzz/src/xv6/fs.img,index=1,media=disk,format=raw -drive file=/home/cwmyung/prj/hyfuzz/src/xv6/build/hyfuzz.img,index=0,media=disk,format=raw -m 512

If you can't reproduce a bug outside of afl-fuzz, be sure to set the same
memory limit. The limit used for this fuzzing session was 0 B.

Need a tool to minimize test cases before investigating the crashes or sending
them to a vendor? Check out the afl-tmin that comes with the fuzzer!

Found any cool bugs in open-source tools using afl-fuzz? If yes, please drop
me a mail at <lcamtuf@coredump.cx> once the issues are fixed - I'd love to
add your finds to the gallery at:

  http://lcamtuf.coredump.cx/afl/

Thanks :-)
