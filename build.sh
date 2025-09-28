#!/bin/bash

# ARX Compiler Build Script
# Builds the fresh compiler implementation

set -e  # Exit on any error

echo "=== ARX Compiler Build Script ==="
echo ""

# Check if we're in the right directory
if [ ! -d "compiler" ]; then
    echo "Error: compiler directory not found"
    echo "Please run this script from the ARX project root"
    exit 1
fi

# Create build directory
echo "Creating build directory..."
mkdir -p build

# Change to build directory
cd build

# Create build structure
echo "Setting up build structure..."
mkdir -p lexer parser codegen

# Build the compiler and install binaries
echo "Building compiler and VM..."
make -f Makefile

echo ""
echo "=== Build Complete ==="
echo "Compiler executable: build/arx"
echo "VM executable: build/arxvm"
echo "Binaries installed to root directory for easy access"
echo ""

# Test the compiler
echo "Testing compiler..."
if [ -f "arx" ]; then
    echo "✅ Compiler built successfully"
    
    # Test with help
    echo "Testing help output..."
    ./arx --help
    
    echo ""
    echo "=== Test Complete ==="
    echo "The ARX compiler is ready to use!"
    echo ""
    echo "Usage examples:"
    echo "  ./arx ../compiler/test/hello.arx"
    echo "  ./arx -debug ../compiler/test/hello.arx"
    echo "  ./arx -o output.arxmod ../compiler/test/hello.arx"
else
    echo "❌ Compiler build failed"
    exit 1
fi
