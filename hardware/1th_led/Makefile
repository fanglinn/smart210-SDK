COMPILE_PATH ?= /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-
CC      = $(COMPILE_PATH)gcc
LD      = $(COMPILE_PATH)ld
AR      = $(COMPILE_PATH)ar
OBJCOPY = $(COMPILE_PATH)objcopy
OBJDUMP = $(COMPILE_PATH)objdump


CFLAGS 		:= -Wall -O2 -I./include -I./src

OBJS := src/led.o \


VER = 1.0
TARGET = smart210_$(VER).bin

$(TARGET): $(OBJS)
	${LD} -Ttext 0xD0020010 -o $(TARGET)_elf $^
	${OBJCOPY} -O binary -S $(TARGET)_elf $@
	${OBJDUMP} -D -m arm $(TARGET)_elf > $(TARGET).dis

	
%.o:%.c
	${CC}  $(CFLAGS) -c -o $@ $<

%.o:%.S
	${CC}  $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(TARGET)_elf $(TARGET).dis *.o $(OBJS)
