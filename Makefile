NT_API_PATH := distingNT_API
INCLUDE_PATH := $(NT_API_PATH)/include
SRC := src/four.cpp
OUTPUT := plugins/four.o

CC := arm-none-eabi-c++
CFLAGS := -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
          -mthumb -fno-rtti -fno-exceptions -Os -fPIC -Wall \
          -I$(INCLUDE_PATH) -Isrc

all: $(OUTPUT)

$(OUTPUT): $(SRC) src/dsp.h
	mkdir -p plugins
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OUTPUT)

.PHONY: all clean
