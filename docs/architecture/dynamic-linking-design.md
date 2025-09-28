# Dynamic Linking Design for ARX Modules

## Overview

This document outlines the design for dynamic linking across multiple ARX module files (`.arxmod`), enabling cross-module class references and method calls.

## Current Implementation

### Unique Class IDs
- Each class is assigned a unique hash ID based on `module_name:class_name`
- This ensures build-time uniqueness within a single module
- Class IDs are stored in the class manifest section of the `.arxmod` file

### Class Manifest Structure
```c
typedef struct {
    char        class_name[32];
    uint64_t    class_id;          // Unique class ID (module:class hash)
    uint32_t    field_count;
    uint32_t    method_count;
    uint64_t    parent_class_id;
    uint32_t    flags;
    uint32_t    reserved;
    // Methods and fields stored inline after this structure
} class_entry_t;
```

## Future Dynamic Linking Design

### 1. External Class References

#### Problem
When a class in Module A needs to reference a class in Module B, we need a way to:
- Identify the external class at compile time
- Resolve the reference at link time
- Load the external class at runtime

#### Solution: External Class Table
Add a new section to the `.arxmod` format:

```c
// External class reference
typedef struct {
    char        module_name[64];   // Source module name
    char        class_name[32];    // Referenced class name
    uint64_t    external_class_id; // Unique ID in external module
    uint32_t    reference_type;    // INHERITANCE, COMPOSITION, etc.
    uint32_t    flags;
} external_class_ref_t;

// External classes section
typedef struct {
    uint32_t    reference_count;
    external_class_ref_t *references;
} external_classes_section_t;
```

### 2. Module Dependencies

#### Module Import System
```c
// Module dependency entry
typedef struct {
    char        module_name[64];   // Imported module name
    char        module_path[256];  // Path to .arxmod file
    uint64_t    module_version;    // Version for compatibility checking
    uint32_t    import_flags;      // Import options
} module_dependency_t;
```

#### Import Declaration Syntax
```arx
module MyModule;

import OtherModule;
import Utils from "path/to/utils.arxmod";

class MyClass extends OtherModule.BaseClass {
    // Class implementation
}
```

### 3. Linker Enhancements

#### Multi-Module Linker
The linker will need to:
1. Parse module dependencies
2. Load external class manifests
3. Resolve cross-module references
4. Generate a unified class table
5. Patch bytecode with resolved addresses

#### Linker Context
```c
typedef struct {
    char                *output_module;     // Output module name
    module_dependency_t *dependencies;      // Module dependencies
    size_t              dependency_count;
    class_entry_t       *unified_classes;   // All classes from all modules
    size_t              class_count;
    uint64_t            base_address;       // Base address for this module
} multi_module_linker_t;
```

### 4. Runtime Class Loading

#### Dynamic Class Resolution
The VM will need to:
1. Load multiple `.arxmod` files
2. Build a unified class table
3. Resolve method calls across modules
4. Handle inheritance across modules

#### VM Enhancements
```c
typedef struct {
    char                *module_name;
    arxmod_reader_t     *reader;
    class_entry_t       *classes;
    size_t              class_count;
    uint64_t            base_address;
} loaded_module_t;

typedef struct {
    loaded_module_t     *modules;
    size_t              module_count;
    class_entry_t       *unified_class_table;
    size_t              total_class_count;
} multi_module_vm_t;
```

### 5. Version Compatibility

#### Module Versioning
```c
typedef struct {
    uint32_t    major_version;
    uint32_t    minor_version;
    uint32_t    patch_version;
    char        build_info[64];
} module_version_t;
```

#### Compatibility Checking
- Major version changes: Breaking changes, incompatible
- Minor version changes: New features, backward compatible
- Patch version changes: Bug fixes, fully compatible

### 6. Security Considerations

#### Module Signing
- Each module can be cryptographically signed
- VM can verify module integrity before loading
- Prevents malicious module injection

#### Sandboxing
- Modules can run in isolated contexts
- Controlled access to system resources
- Inter-module communication through defined interfaces

## Implementation Phases

### Phase 1: External Class References (Current)
- ✅ Unique class IDs based on module:class names
- ✅ Class manifest with unique IDs
- 🔄 Design external class reference structure

### Phase 2: Module Dependencies
- Import declaration parsing
- Module dependency tracking
- Basic cross-module class resolution

### Phase 3: Multi-Module Linker
- Enhanced linker for multiple modules
- Unified class table generation
- Cross-module method resolution

### Phase 4: Runtime Multi-Module Support
- VM support for multiple loaded modules
- Dynamic class loading
- Cross-module method calls

### Phase 5: Advanced Features
- Module versioning and compatibility
- Module signing and security
- Performance optimizations

## Benefits

1. **Modularity**: Code can be organized into logical modules
2. **Reusability**: Common classes can be shared across projects
3. **Maintainability**: Changes to one module don't affect others
4. **Scalability**: Large projects can be split into manageable modules
5. **Team Development**: Different teams can work on different modules

## Challenges

1. **Complexity**: Dynamic linking adds significant complexity
2. **Performance**: Cross-module calls may be slower
3. **Debugging**: Harder to debug across module boundaries
4. **Dependency Management**: Need to handle circular dependencies
5. **Version Management**: Ensuring compatible module versions

## Conclusion

The unique class ID system provides a solid foundation for future dynamic linking capabilities. The phased approach allows for incremental implementation while maintaining system stability.
