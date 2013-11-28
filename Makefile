obj-m := rbping.o
PWD := $(shell pwd)

all:
	make -C /lib/modules/`uname -r`/build SUBDIRS=/root/rbping/ modules
	make -C /lib/modules/`uname -r`/build SUBDIRS=/root/rbping/ modules_install
	depmod -a
	
