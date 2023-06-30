.PHONY: module clean server.c server.o client.c client.o
obj-m += server.o client.o
PROG = module
ccflags-y := -std=gnu99 -Wno-declaration-after-statement

all: $(PROG)
module:
	make -I/home/vagrant/HomaModule -C /lib/modules/$(shell uname -r)/build M=$(PWD) modules
clean:
	(rm -rf *.o *.ko modules.order test.mod.c Module.symvers kern_homa_server_test.mod )
