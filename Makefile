# Makefile to build light_ws2812 library examples
# This is not a very good example of a makefile - the dependencies do not work, therefore everything is rebuilt every time.

# Change these parameters for your device

F_CPU = 16000000
DEVICE = atmega328p

# Tools:
CC = avr-gcc

LIB       = light_ws2812
EXAMPLES  = tvpatterns
DEP		  = ws2812_config.h light_ws2812.h

CFLAGS = -g2 -I. -ILight_WS2812 -mmcu=$(DEVICE) -DF_CPU=$(F_CPU) 
CFLAGS+= -Os -ffunction-sections -fdata-sections -fpack-struct -fno-move-loop-invariants -fno-tree-scev-cprop -fno-inline-small-functions  
CFLAGS+= -Wall -Wno-pointer-to-int-cast
#CFLAGS+= -Wa,-ahls=$<.lst

LDFLAGS = -Wl,--relax,--section-start=.text=0,-Map=main.map

all:	$(EXAMPLES) 

$(LIB): $(DEP)
	@echo Building Library 
	@$(CC) $(CFLAGS) -o obj/$@.o -c $@.c 

$(EXAMPLES): $(LIB) 
	@echo Building $@
	@$(CC) $(CFLAGS) -o obj/$@.o $@.c $^.c
	@avr-size obj/$@.o
	@avr-objcopy -j .text  -j .data -O ihex obj/$@.o $@.hex
	@avr-objdump -d -S obj/$@.o >obj/$@.lss

.PHONY:	clean

clean:
	rm -f *.hex obj/*.o obj/*.lss
