.PHONY: module clean kern_homa_server_test.c kern_homa_server_test.o kern_homa_client_test.c kern_homa_client_test.o
obj-m += kern_homa_server_test.o kern_homa_client_test.o
PROG = module
CFLAGS=-std=c99
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all: $(PROG)
module:
	make -I/home/vagrant/HomaModule -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	(rm -rf *.o *.ko modules.order test.mod.c Module.symvers kern_homa_server_test.mod )
