CC=gcc
CFLAGS=-pthread -Wall -std=c99
TARGET=bakery

all: $(TARGET)

$(TARGET): main.c
	$(CC) $(CFLAGS) -o $(TARGET) main.c

clean:
	rm -f $(TARGET)
