CC      = /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-gcc
LD      = /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-ld
AR      = /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-ar
OBJCOPY = /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-objcopy
OBJDUMP = /home/flinn/tools/4.5.1/bin/arm-none-linux-gnueabi-objdump


CFLAGS          := -Wall -O2 -nostdlib -fno-builtin \
		-I./include \
		-I./HAL \
		-I./common \
		-I./src

OBJS := src/head.o src/main.o 

VER = 1.0
TARGET = smart210_$(VER).bin

$(TARGET): $(OBJS)
	${LD} -Tbasic.lds -o $(TARGET)_elf $^
	${OBJCOPY} -O binary -S $(TARGET)_elf $@
	${OBJDUMP} -D -m arm $(TARGET)_elf > $(TARGET).dis


%.o:%.c
	${CC}  $(CFLAGS) -c -o $@ $<

%.o:%.S
	${CC}  $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(TARGET) $(TARGET)_elf $(TARGET).dis *.o *.bin $(OBJS)
