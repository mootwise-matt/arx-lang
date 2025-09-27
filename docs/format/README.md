# ARX File Formats

This directory contains documentation for the various file formats used by the ARX programming language and its toolchain.

## Formats

### ARX Module Format (.arxmod)
- **File**: [arxmod-format.md](arxmod-format.md)
- **Description**: Binary format for compiled ARX programs
- **Purpose**: Store bytecode, metadata, and debugging information
- **Tools**: `arxmod_info` for inspection and validation

## Quick Reference

### ARX Module Format
```bash
# Compile ARX source to module
arx program.arx

# Inspect module information
arxmod_info program.arxmod

# Show section details
arxmod_info -sections program.arxmod

# Validate module format
arxmod_info -validate program.arxmod

# Show hex dump of header
arxmod_info -hex program.arxmod
```

## File Structure Overview

```
ARX Module (.arxmod)
├── Header (64 bytes)
│   ├── Magic: "ARXMOD\0\0"
│   ├── Version: 1
│   ├── TOC offset/size
│   └── Data offset/size
├── Table of Contents
│   ├── Section entries
│   └── Section metadata
└── Data Sections
    ├── CODE (bytecode)
    ├── STRINGS (literals)
    ├── SYMBOLS (debug info)
    ├── DEBUG (line mapping)
    └── APP (metadata)
```

## Tools

### arxmod_info
Command-line tool for inspecting ARX module files.

**Usage:**
```bash
arxmod_info [options] <arxmod_file>
```

**Options:**
- `-info` - Show module information (default)
- `-sections` - Show section details
- `-validate` - Validate file format
- `-hex` - Show hex dump of header
- `-h, --help` - Show help message

**Examples:**
```bash
# Basic information
arxmod_info program.arxmod

# Detailed section information
arxmod_info -sections program.arxmod

# Validate file integrity
arxmod_info -validate program.arxmod

# Debug header format
arxmod_info -hex program.arxmod
```

## Format Specifications

### Header Format
- **Size**: 64 bytes
- **Magic**: "ARXMOD\0\0" (8 bytes)
- **Version**: 1 (4 bytes)
- **Flags**: Reserved (4 bytes)
- **TOC offset/size**: Table of Contents location (16 bytes)
- **Data offset/size**: Data sections location (16 bytes)
- **App info**: Application metadata (16 bytes)

### Section Types
- **CODE**: Compiled bytecode instructions
- **STRINGS**: String literals and identifiers
- **SYMBOLS**: Symbol table for debugging
- **DEBUG**: Source line mapping
- **APP**: Application-specific data

### Byte Order
All multi-byte values are stored in **little-endian** format.

### Alignment
- Header: 64 bytes (fixed)
- TOC entries: 8-byte aligned
- Data sections: 16-byte aligned

## Validation

A valid ARX module file must:
1. Start with magic number "ARXMOD\0\0"
2. Have version number 1
3. Have header size of 64 bytes
4. Have valid TOC offset and size
5. Have valid data offset and size
6. Have properly aligned sections

## Future Extensions

The format is designed to be extensible. Future versions may include:
- Compression for smaller file sizes
- Encryption for security
- Module dependencies
- Embedded resources
- Additional metadata sections
- Optimized bytecode variants
