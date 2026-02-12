CC := c++
CFLAGS := -std=c++11 -Wall -Wextra -g -fsanitize=address,undefined
SRC := test_dsp.cpp
OUTPUT := test_dsp

all: $(OUTPUT)

$(OUTPUT): $(SRC) ../dsp.h
	$(CC) $(CFLAGS) -o $@ $< -lm

run: $(OUTPUT)
	./$(OUTPUT)

clean:
	rm -f $(OUTPUT)

.PHONY: all run clean
