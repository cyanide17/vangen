
/home/cwmyung/prj/hyfuzz/src/qemu-5.2/build/i386-softmmu/qemu-system-i386 -m 512 -drive file=/home/cwmyung/prj/hyfuzz/src/xv6/fs.img,index=1,media=disk,format=raw -drive file=/home/cwmyung/prj/hyfuzz/src/xv6/build/hyfuzz.img,index=0,media=disk,format=raw -device am53c974,id=scsi -device scsi-hd,drive=SysDisk -drive id=SysDisk,if=none,file=/home/cwmyung/prj/hyfuzz/src/image/block/disk.img


