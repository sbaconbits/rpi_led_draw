CC=/usr/bin/avr-gcc
OBJCOPY=/usr/bin/avr-objcopy
ALL_FLAGS = -mmcu=atmega328p

C_DEFS = -DF_CPU=20000000UL

C_FLAGS = $(ALL_FLAGS)
C_FLAGS += -Os
C_FLAGS += -g -Wa,-ahlms=$(<:.c=.lst)
C_FLAGS += $(C_DEFS)

#AVRDUDE = avrdude
AVRDUDE = /store/bin/avrdude-5.11/avrdude

all: main.hex size

leds.o: leds.S
	$(CC) $(ALL_FLAGS) -c leds.S

main.o: main.c
	$(CC) $(C_FLAGS) -c main.c

main.elf: main.o leds.o
	$(CC) $(ALL_FLAGS) -o main.elf main.o leds.o

main.hex: main.elf
	$(OBJCOPY) -j .text -j .data -O ihex main.elf main.hex

prog:
	$(AVRDUDE) -p m328p -c avrispmkII -P usb -U flash:w:main.hex -v

size:
	avr-size --format=avr --mcu=atmega328p main.elf

# Fuse low byte:
# CKDIV8	7    1      No divide by 8
# CKOUT		6    1      No clock out
# SUT		5:4  10     Start up time set to max
# CKSEL		3:0  1110   low power crystal (not sure about CKSEL 0)??
# 1110 1110 = ee

fuse:
	$(AVRDUDE) -p m328p -c avrispmkII -P usb -U lfuse:w:0xee:m -v

clean:
	rm -f *.elf *.hex *.lst *.o
 


