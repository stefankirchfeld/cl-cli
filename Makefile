CC      = cc
CFLAGS  = -O2 -Wall -Isrc $(shell pkg-config --cflags jansson)
LDFLAGS = $(shell pkg-config --libs jansson) -lcurl
SRCS    = $(wildcard src/*.c)

build/cl-bin: $(SRCS)
	mkdir -p build
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

clean:
	rm -f build/cl-bin

.PHONY: clean
