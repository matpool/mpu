obj-m += mpu.o 
mpu-objs := src/mpu_drv.o src/mpu_syscall_hook.o src/mpu_ioctl.o

all:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules

clean:
	make -C /lib/modules/$(shell uname -r)/build M=$(PWD) clean
