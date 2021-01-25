obj-m := mod.o
 
#KDIR := /lib/modules/$(shell uname -r)/build
KDIR := /home/sjkim/Desktop/hypervisor-fuzz/build/linux-5.8.0
PWD := $(shell pwd)
 
default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
 
clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

