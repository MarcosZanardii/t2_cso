obj-m := pubsub_driver.o
pubsub_driver-objs := main_driver.o broker.o
BUILDROOT_DIR := ../..
KDIR := $(BUILDROOT_DIR)/output/build/linux-custom
COMPILER := $(BUILDROOT_DIR)/output/host/bin/i686-buildroot-linux-gnu-gcc

all:
	$(MAKE) -C $(KDIR) M=$$PWD
	$(MAKE) -C $(KDIR) M=$$PWD modules_install INSTALL_MOD_PATH=../../target
	$(COMPILER) -o test_pubsub_driver test_pubsub_driver.c
	cp test_pubsub_driver $(BUILDROOT_DIR)/output/target/bin
	
clean:
	rm -f *.o *.ko .*.cmd
	rm -f modules.order
	rm -f Module.symvers
	rm -f pubsub_driver.mod.c
	rm -f test_pubsub_driver
