HOME	:=/home/hiro/artik530_Ubuntu
ARCH	:=arm
CROSS_COMPILE	:=arm-linux-gnueabihf-
KPATH	:=$(HOME)/linux-artik
PWD	:=$(shell pwd)

CFILES	:=myDeviceDriver_interrupt.c
obj-m	:=MyDeviceModule_interrupt.o
MyDeviceModule_interrupt-objs	:=$(CFILES:.c=.o)

ccflags-y += -std=gnu99 -Wall -Wno-declaration-after-statement


all:    
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KPATH) M=$(PWD) modules

clean:  
	make ARCH=$(ARCH) CROSS_COMPILE=$(CROSS_COMPILE) -C $(KPATH) M=$(PWD) clean                                                                       
