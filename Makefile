TARGET = avr-donut

# mcu + hw config
MCU = atmega328p
F_CPU = 16000000UL
PORT = /dev/ttyACM0
BAUD = 115200

# compiler and programmer stuff
CC = avr-gcc
OBJCOPY = avr-objcopy
AVRDUDE = avrdude
AVRSIZE = avr-size

# -O3 optimisation flags
CFLAGS = -Wall -O3 -mmcu=$(MCU) -DF_CPU=$(F_CPU) -std=gnu99 \
         -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums \
         -ffunction-sections -fdata-sections \
         -fno-inline-small-functions -fno-tree-loop-distribute-patterns
         
LDFLAGS = -Wl,--gc-sections -mmcu=$(MCU)

# src code
SRCS = avr-donut.c

# make all
all: $(TARGET).hex
	@echo "================ avr-size stuff ================"
	@$(AVRSIZE) --format=avr --mcu=$(MCU) $(TARGET).elf
	@echo "================================================"

$(TARGET).elf: $(SRCS)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $^

$(TARGET).hex: $(TARGET).elf
	$(OBJCOPY) -O ihex -R .eeprom $< $@

# make flash
flash: $(TARGET).hex
	$(AVRDUDE) -F -V -c arduino -p $(MCU) -P $(PORT) -b $(BAUD) -U flash:w:$<:i

# make clean
clean:
	rm -f $(TARGET).elf $(TARGET).hex
.PHONY: all flash clean
