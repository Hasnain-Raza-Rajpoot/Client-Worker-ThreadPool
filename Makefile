# Makefile for Generic Queue Implementation

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
TARGET = test_queue
OBJECTS = queue.o test_queue.o

# Default target
all: $(TARGET)

# Build the test program
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS)

# Compile queue.c
queue.o: queue.c queue.h
	$(CC) $(CFLAGS) -c queue.c

# Compile test_queue.c
test_queue.o: test_queue.c queue.h
	$(CC) $(CFLAGS) -c test_queue.c

# Run the test program
run: $(TARGET)
	./$(TARGET)

# Clean up object files and executable
clean:
	rm -f $(OBJECTS) $(TARGET)

# Clean and rebuild
rebuild: clean all

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: $(TARGET)

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: clean $(TARGET)

# Install (copy to /usr/local/bin - requires sudo)
install: $(TARGET)
	sudo cp $(TARGET) /usr/local/bin/

# Uninstall
uninstall:
	sudo rm -f /usr/local/bin/$(TARGET)

# Help
help:
	@echo "Available targets:"
	@echo "  all      - Build the test program (default)"
	@echo "  run      - Build and run the test program"
	@echo "  clean    - Remove object files and executable"
	@echo "  rebuild  - Clean and rebuild"
	@echo "  debug    - Build with debug flags"
	@echo "  release  - Build with optimization flags"
	@echo "  install  - Install to /usr/local/bin (requires sudo)"
	@echo "  uninstall- Remove from /usr/local/bin"
	@echo "  help     - Show this help message"

.PHONY: all run clean rebuild debug release install uninstall help
