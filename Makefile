# Makefile for Giri Shell (gsh)

CC = gcc
CFLAGS = -Wall -Wextra -g
TARGET = shell
SRC :=  ./src/shell.c \
		./src/input.c

all: $(TARGET)

$(TARGET): $(SRC)
	$(CC) $(CFLAGS) -o $(TARGET) $(SRC)

clean:
	rm -f $(TARGET)
