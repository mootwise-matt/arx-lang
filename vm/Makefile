# ARX Virtual Machine Makefile
# Builds the ARX VM executable

CC = clang
CFLAGS = -std=c99 -Wall -Wextra -O2 -g -I../compiler -I.
LDFLAGS = 
TARGET = ../arxvm
BUILD_DIR = ../build/vm

# Source files
SOURCES = arxvm.c \
          core/vm.c \
          loader/loader.c \
          runtime/runtime.c

# ARX module files (from compiler)
ARXMOD_SOURCES = ../compiler/arxmod/arxmod_reader.c \
                 ../compiler/arxmod/arxmod_writer.c

# Object files
OBJECTS = $(SOURCES:%.c=$(BUILD_DIR)/%.o) \
          $(ARXMOD_SOURCES:../compiler/%.c=$(BUILD_DIR)/%.o)

# Default target
all: $(TARGET)

# Build the VM
$(TARGET): $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LDFLAGS)
	@echo "Build complete: $(TARGET)"

# Compile source files
$(BUILD_DIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Compile ARX module files
$(BUILD_DIR)/arxmod/%.o: ../compiler/arxmod/%.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

# Clean build artifacts
clean:
	rm -rf $(BUILD_DIR)
	rm -f $(TARGET)
	@echo "Clean complete"

# Install the VM
install: $(TARGET)
	cp $(TARGET) /usr/local/bin/
	@echo "VM installed to /usr/local/bin/"

# Uninstall the VM
uninstall:
	rm -f /usr/local/bin/$(TARGET)
	@echo "VM uninstalled from /usr/local/bin/"

# Test the VM
test: $(TARGET)
	@echo "Testing VM with sample program..."
	./$(TARGET) -debug ../compiler/test/hello.arxmod

# Debug build
debug: CFLAGS += -DDEBUG -g3
debug: $(TARGET)

# Release build
release: CFLAGS += -O3 -DNDEBUG
release: $(TARGET)

# Show build information
info:
	@echo "=== ARX VM Build Information ==="
	@echo "Compiler: $(CC)"
	@echo "Flags: $(CFLAGS)"
	@echo "Sources: $(SOURCES)"
	@echo "Objects: $(OBJECTS)"
	@echo "Target: $(TARGET)"

# Create build directory structure
setup:
	@mkdir -p $(BUILD_DIR)/core
	@mkdir -p $(BUILD_DIR)/loader
	@mkdir -p $(BUILD_DIR)/runtime
	@mkdir -p $(BUILD_DIR)/arxmod
	@echo "Build directory structure created"

.PHONY: all clean install uninstall test debug release info setup
