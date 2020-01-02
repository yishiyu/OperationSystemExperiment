obj-m := bibibi_syscall.o
KERNELDIR := /lib/modules/$(shell uname -r)/build
PWD := $(shell pwd)
CONFIG_MODULE_SIG=n

modules:
	$(MAKE) -C $(KERNELDIR) M=$(PWD) modules


clean:
	 rm -rf *.o *~ core .depend .*.cmd *.ko *.mod.c .tmp_versions