# Define the compiler and the flags
CC = gcc
CFLAGS = -Wall -g

# Define the source files and the executable name
SOURCE = shell1.c
EXECUTABLE = myshell

# Define the default target
all: $(EXECUTABLE)

# Rule to build the executable from shell1.c
$(EXECUTABLE): $(SOURCE)
	$(CC) $(CFLAGS) $(SOURCE) -o $(EXECUTABLE)

# Clean rule to remove the executable
clean:
	rm -f $(EXECUTABLE)
