obj-m := lab_sensor.o
all:
	$(MAKE) -C $(KDIR) M=$(PWD) modules
clean:
	rm -f *.o *.ko *.mod.*
