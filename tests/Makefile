CC := c++
CFLAGS := -std=c++11 -Wall -Wextra -g -I../src -fsanitize=address,undefined
SRC := test_dsp.cpp
OUTPUT := test_dsp

all: $(OUTPUT)

$(OUTPUT): $(SRC) ../src/dsp.h
	$(CC) $(CFLAGS) -o $@ $< -lm

run: $(OUTPUT)
	./$(OUTPUT)

clean:
	rm -f $(OUTPUT)

.PHONY: all run clean
