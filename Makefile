# the compiler to use
CC = gcc

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -Wall --libs gtk+-3.0

#files to link:
LFLAGS = -lasound

# the name to use for both the target source file, and the output file:
TARGET = midi-pipes

all: $(TARGET)

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $(TARGET) $(TARGET).c $(LFLAGS)
