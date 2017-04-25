.SUFFIXES : .x .o .c .s

CC := arm-linux-gcc
STRIP := arm-linux-strip

TARGET = uart_demo
SRCS := uart.c
LIBS = -lpthread -lc -lgcc

KERNEL_DIR=../../nuc970bsp/linux-3.10.x
INSTALL_PATH=../../nuc970bsp/rootfs_48
all:
	$(CC) -static $(SRCS) -o $(TARGET) $(LIBS)
	$(STRIP) $(TARGET)
	$(CC) send.c -o send
	$(CC) charset.c -o charset
	$(CC) demo1.c -o demo1

clean:
	rm -f *.o
	rm -f *.x
	rm -f *.flat
	rm -f *.map
	rm -f temp
	rm -f *.img
	rm -f $(TARGET)
	rm -f *.gdb
