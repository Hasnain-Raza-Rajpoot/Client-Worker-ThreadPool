# Makefile for Multi-threaded File Storage Server

CC = gcc
CFLAGS = -Wall -Wextra -std=c99 -g
LDFLAGS = -pthread -lsqlite3 -lm

SERVER_TARGET = server
CLIENT_TARGET = client

# List server and client source files
SERVER_C_FILES = main.c queue.c storage.c file_locks.c utility.c
CLIENT_C_FILES = test_client.c

# Automatically generate object file names
SERVER_OBJECTS = $(SERVER_C_FILES:.c=.o)
CLIENT_OBJECTS = $(CLIENT_C_FILES:.c=.o)

# Default target: build both
all: $(SERVER_TARGET) $(CLIENT_TARGET)

# Build the server executable
$(SERVER_TARGET): $(SERVER_OBJECTS)
	$(CC) $(CFLAGS) -o $(SERVER_TARGET) $(SERVER_OBJECTS) $(LDFLAGS)

# Build the client executable
$(CLIENT_TARGET): $(CLIENT_OBJECTS)
	$(CC) $(CFLAGS) -o $(CLIENT_TARGET) $(CLIENT_OBJECTS) $(LDFLAGS)

# Generic rule for compiling .c files into .o files
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# Run the server program
run_server: $(SERVER_TARGET)
	./$(SERVER_TARGET)

# Run the client program
run_client: $(CLIENT_TARGET)
	./$(CLIENT_TARGET)

# Clean up object files and executables
clean:
	rm -f $(SERVER_OBJECTS) $(CLIENT_OBJECTS) $(SERVER_TARGET) $(CLIENT_TARGET) test_queue test_storage

# Clean and rebuild
rebuild: clean all

# Debug build
debug: CFLAGS += -DDEBUG -O0
debug: $(SERVER_TARGET) $(CLIENT_TARGET)

# Release build
release: CFLAGS += -O2 -DNDEBUG
release: clean $(SERVER_TARGET) $(CLIENT_TARGET)

# ==========================================================
# Sanitizers (ThreadSanitizer and AddressSanitizer)
# ==========================================================

TSAN_FLAGS = -fsanitize=thread -fno-omit-frame-pointer
ASAN_FLAGS = -fsanitize=address -fno-omit-frame-pointer

# ThreadSanitizer builds
tsan: CFLAGS += $(TSAN_FLAGS)
tsan: LDFLAGS += $(TSAN_FLAGS)
tsan: clean $(SERVER_TARGET) $(CLIENT_TARGET)

# AddressSanitizer builds
asan: CFLAGS += $(ASAN_FLAGS)
asan: LDFLAGS += $(ASAN_FLAGS)
asan: clean $(SERVER_TARGET) $(CLIENT_TARGET)

# ==========================================================
# Valgrind helpers
# ==========================================================

VALGRIND = valgrind --leak-check=full --show-leak-kinds=all --track-origins=yes

valgrind_server: $(SERVER_TARGET)
	$(VALGRIND) ./$(SERVER_TARGET)

valgrind_client: $(CLIENT_TARGET)
	$(VALGRIND) ./$(CLIENT_TARGET)

# Help
help:
	@echo "Available targets:"
	@echo "  all        - Build server and client (default)"
	@echo "  run_server - Build and run the server"
	@echo "  run_client - Build and run the client"
	@echo "  clean      - Remove object files and executables"
	@echo "  rebuild    - Clean and rebuild"
	@echo "  debug      - Build with debug flags"
	@echo "  release    - Build with optimization flags"
	@echo "  help       - Show this help message"

.PHONY: all run_server run_client clean rebuild debug release help