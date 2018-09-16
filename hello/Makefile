obj-m:=hello1.o
CURRENT_PATH:=$(shell pwd)
LINUX_KERNEL_PATH:=/usr/src/linux-headers-4.4.0-135-generic
all:
	make -C  $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) modules
	gcc test.c -o test
	gcc test2.c -o test2
clean:
	make -C  $(LINUX_KERNEL_PATH) M=$(CURRENT_PATH) clean
	rm -rf test test2
