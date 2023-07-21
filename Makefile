.PHONY: module clean server.c server.o client.c client.o zcserver.c zcserver.o zcclient.c zcclient.o
obj-m += server.o client.o zcserver.o zcclient.o
PROG = module
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all: $(PROG)
module:
	make -I/home/vagrant/HomaModule0 -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	(rm -rf *.o *.ko modules.order *.mod.c *.mod.o Module.symvers kern_homa_server_test.mod *.cmd)
