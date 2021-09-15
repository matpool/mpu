obj-m += mpu.o
mpu-objs := src/mpu_drv.o src/mpu_syscall_hook.o src/mpu_ioctl.o

KVERSION := $(shell uname -r)
KDIR := /lib/modules/$(KVERSION)/build
PWD := $(shell pwd)

default:
	$(MAKE) -C $(KDIR) M=$(PWD) modules

clean:
	$(MAKE) -C $(KDIR) M=$(PWD) clean

install:
	$(MAKE) -C $(KDIR) M=$(PWD) modules_install
	depmod
	insmod mpu.ko
	echo mpu > /etc/modules-load.d/matpool-mpu.conf

uninstall:
	rmmod mpu.ko
	depmod
	rm /etc/modules-load.d/matpool-mpu.conf
