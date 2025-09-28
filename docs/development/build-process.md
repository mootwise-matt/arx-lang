# ARX Build Process Documentation

## Overview
The ARX project uses a structured build process that compiles binaries in the `build/` directory and automatically installs them to the root directory for easy access.

## Build Architecture

### Directory Structure
```
arx/
├── build/           # Build directory (temporary)
│   ├── Makefile     # Build configuration
│   ├── arx          # Compiler binary (built here)
│   └── arxvm        # VM binary (built here)
├── arx              # Compiler binary (installed here)
├── arxvm            # VM binary (installed here)
├── build.sh         # Main build script
└── compiler/        # Source code
```

### Build Process Flow
1. **Source Compilation**: Binaries are built in `build/` directory
2. **Automatic Installation**: Binaries are copied to root directory
3. **Easy Access**: Users can run `./arx` and `./arxvm` from root

## Building ARX

### Quick Build
```bash
# From project root
./build.sh
```

### Manual Build
```bash
# From project root
cd build
make
```

### Build Targets
- `make all` - Builds compiler, VM, and installs to root
- `make arx` - Builds only the compiler
- `make arxvm` - Builds only the VM
- `make install` - Installs existing binaries to root
- `make clean` - Removes all build artifacts

## Important Notes

### ⚠️ Binary Synchronization
- **Always use root binaries** (`./arx`, `./arxvm`) for running examples
- **Build directory binaries** are intermediate - don't use them directly
- **After any build**, root binaries are automatically updated

### 🔄 Development Workflow
1. Make code changes
2. Run `./build.sh` or `cd build && make`
3. Binaries are automatically installed to root
4. Test with `./arx` and `./arxvm` from root directory

### 🚨 Common Issues

#### "Invalid Address" Errors
- **Cause**: Using outdated binaries
- **Solution**: Rebuild with `./build.sh` or `cd build && make`
- **Prevention**: Always use root binaries after building

#### Binary Mismatch
- **Cause**: Manual copying or partial builds
- **Solution**: Clean build with `cd build && make clean && make`
- **Prevention**: Use automated build process

## Build Configuration

### Makefile Features
- **Automatic Installation**: `install` target copies binaries to root
- **Dependency Tracking**: Rebuilds only changed files
- **Clean Target**: Removes all build artifacts
- **Cross-Platform**: Uses standard GCC with C99

### Compiler Flags
```makefile
CFLAGS = -Wall -Wextra -std=c99 -g -O2
```
- `-Wall -Wextra`: Enable all warnings
- `-std=c99`: Use C99 standard
- `-g`: Include debug symbols
- `-O2`: Optimize for performance

## Testing Build

### Verify Installation
```bash
# Check binary versions
ls -la arx arxvm

# Test compilation
./arx examples/01_hello_world.arx

# Test execution
./arxvm examples/01_hello_world.arxmod
```

### Run All Examples
```bash
# Test all examples
for example in examples/*.arx; do
    echo "Testing $example"
    ./arx "$example"
    ./arxvm "${example%.arx}.arxmod"
done
```

## Troubleshooting

### Build Fails
1. Check for syntax errors in source code
2. Ensure all dependencies are installed
3. Try clean build: `cd build && make clean && make`

### Runtime Errors
1. Verify you're using root binaries (`./arx`, `./arxvm`)
2. Check if binaries are up to date
3. Rebuild if necessary: `./build.sh`

### Permission Issues
```bash
# Make scripts executable
chmod +x build.sh
chmod +x arx arxvm
```

## Best Practices

### For Developers
- Always rebuild after code changes
- Use `make clean` before major changes
- Test with examples after building
- Commit updated binaries when needed

### For Users
- Use root binaries (`./arx`, `./arxvm`)
- Rebuild if you encounter errors
- Check build documentation for issues
- Report build problems with full error messages

## Future Improvements

### Planned Enhancements
- **Cross-compilation**: Support for different architectures
- **Package Management**: Automated dependency handling
- **CI/CD Integration**: Automated testing and deployment
- **Debug Builds**: Separate debug and release configurations

### Build Optimization
- **Parallel Compilation**: Use `make -j` for faster builds
- **Incremental Builds**: Only rebuild changed components
- **Binary Stripping**: Remove debug symbols for release builds
