ifneq ($(KERNELRELEASE),)
	obj-m := my_timer.o
else
	KERNELDIR ?= /lib/modules/`uname -r`/build/
	PWD := `pwd`
default:
	make -C $(KERNELDIR) M=$(PWD) modules
endif

clean:
	rm -f *.ko *.o Module* *mod*
