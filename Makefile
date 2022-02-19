# project name (generate executable with this name)
TARGET := src/trinityenabler

# compiling flags here
CFLAGS := -std=c99 -Wall -I. -pedantic -Werror -O3 $(shell pkg-config --cflags libusb-1.0)

# linking flags here
LFLAGS := -Wall -I. $(shell pkg-config --libs libusb-1.0)

OBJS := src/main.o
rm := rm -f


$(TARGET): $(OBJS)
	$(CC) $(LFLAGS) -o $@ $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

.PHONY: clean
clean:
	$(rm) $(OBJS)

.PHONY: remove
remove: clean
	$(rm) $(TARGET)
