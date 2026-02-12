NT_API_PATH := distingNT_API
INCLUDE_PATH := $(NT_API_PATH)/include
SRC := src/four.cpp
OUTPUT := plugins/four.o
MANIFEST := plugins/plugin.json
VERSION := $(shell cat VERSION)

CC := arm-none-eabi-c++
CFLAGS := -std=c++11 -mcpu=cortex-m7 -mfpu=fpv5-d16 -mfloat-abi=hard \
          -mthumb -fno-rtti -fno-exceptions -Os -fPIC -Wall \
          -I$(INCLUDE_PATH) -Isrc \
          -DFOUR_VERSION='"$(VERSION)"'

all: $(OUTPUT) $(MANIFEST)

$(OUTPUT): $(SRC) src/dsp.h VERSION
	mkdir -p plugins
	$(CC) $(CFLAGS) -c -o $@ $<

$(MANIFEST): VERSION
	mkdir -p plugins
	@echo '{"name": "Four", "guid": "9df75c7a-87db-4d63-b414-24c50939029b", "version": "$(VERSION)", "author": "wintoid", "description": "Four - 4-op FM synthesizer for Disting NT", "tags": ["instrument", "synth", "FM"]}' > $@

clean:
	rm -f $(OUTPUT) $(MANIFEST)

.PHONY: all clean
