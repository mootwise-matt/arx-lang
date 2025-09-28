/*
 * ARX Module Information Tool
 * Displays information about .arxmod files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../arxmod/arxmod.h"
#include "../common/arxmod_constants.h"

// Global debug flag for tools
bool debug_mode = false;

void print_usage(const char* program_name)
{
    printf("ARX Module Information Tool v1.0\n");
    printf("Usage: %s [options] <arxmod_file>\n", program_name);
    printf("\n");
    printf("Options:\n");
    printf("  -info          Show module information (default)\n");
    printf("  -sections      Show section details\n");
    printf("  -classes       Show class information\n");
    printf("  -validate      Validate file format\n");
    printf("  -hex           Show hex dump of header\n");
    printf("  -h, --help     Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s module.arxmod\n", program_name);
    printf("  %s -sections module.arxmod\n", program_name);
    printf("  %s -classes module.arxmod\n", program_name);
    printf("  %s -validate module.arxmod\n", program_name);
    printf("\n");
}

int main(int argc, char *argv[])
{
    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }
    
    char *filename = NULL;
    bool show_info = true;
    bool show_sections = false;
    bool show_classes = false;
    bool validate_only = false;
    bool show_hex = false;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-info") == 0) {
            show_info = true;
        }
        else if (strcmp(argv[i], "-sections") == 0) {
            show_sections = true;
            show_info = false;
        }
        else if (strcmp(argv[i], "-classes") == 0) {
            show_classes = true;
            show_info = false;
        }
        else if (strcmp(argv[i], "-validate") == 0) {
            validate_only = true;
            show_info = false;
        }
        else if (strcmp(argv[i], "-hex") == 0) {
            show_hex = true;
            show_info = false;
        }
        else if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        }
        else if (argv[i][0] != '-') {
            filename = argv[i];
        }
        else {
            printf("Error: Unknown option '%s'\n", argv[i]);
            print_usage(argv[0]);
            return 1;
        }
    }
    
    if (filename == NULL) {
        printf("Error: No input file specified\n");
        print_usage(argv[0]);
        return 1;
    }
    
    // Validate file
    if (!arxmod_validate_file(filename)) {
        printf("Error: '%s' is not a valid ARX module file\n", filename);
        return 1;
    }
    
    if (validate_only) {
        printf("'%s' is a valid ARX module file\n", filename);
        return 0;
    }
    
    // Open and read module
    arxmod_reader_t reader;
    if (!arxmod_reader_init(&reader, filename)) {
        printf("Error: Could not open file '%s'\n", filename);
        return 1;
    }
    
    if (!arxmod_reader_validate(&reader)) {
        printf("Error: File validation failed\n");
        arxmod_reader_cleanup(&reader);
        return 1;
    }
    
    if (!arxmod_reader_load_toc(&reader)) {
        printf("Error: Could not load table of contents\n");
        arxmod_reader_cleanup(&reader);
        return 1;
    }
    
    // Show requested information
    if (show_info) {
        arxmod_dump_info(&reader);
    }
    
    if (show_sections) {
        arxmod_dump_sections(&reader);
    }
    
    if (show_classes) {
        // Load and display class information
        class_entry_t *classes = NULL;
        size_t class_count = 0;
        
        method_entry_t *methods = NULL;
        size_t method_count = 0;
        field_entry_t *fields = NULL;
        size_t field_count = 0;
        
        printf("DEBUG: About to load classes section...\n");
        if (arxmod_reader_load_classes_section(&reader, &classes, &class_count, &methods, &method_count, &fields, &field_count)) {
            printf("DEBUG: Classes section loaded successfully\n");
            printf("\n=== ARX Module Classes ===\n");
            if (class_count == 0) {
                printf("No classes found in module.\n");
            } else {
                printf("Found %zu classes:\n\n", class_count);
                for (size_t i = 0; i < class_count; i++) {
                    printf("Class %zu: %s\n", i + 1, classes[i].class_name);
                    printf("  ID: %llu\n", (unsigned long long)classes[i].class_id);
                    printf("  Fields: %u\n", classes[i].field_count);
                    printf("  Methods: %u\n", classes[i].method_count);
                    printf("  Parent ID: %llu\n", (unsigned long long)classes[i].parent_class_id);
                    printf("  Flags: 0x%08X\n", classes[i].flags);
                    printf("\n");
                }
            }
            
            // Display methods with signature information
            if (method_count > 0) {
                printf("\n=== ARX Module Methods ===\n");
                printf("Found %zu methods:\n\n", method_count);
                for (size_t i = 0; i < method_count; i++) {
                    printf("Method %zu: %s\n", i + 1, methods[i].method_name);
                    printf("  ID: %llu\n", (unsigned long long)methods[i].method_id);
                    printf("  Offset: %llu\n", (unsigned long long)methods[i].offset);
                    printf("  Parameters: %u\n", methods[i].parameter_count);
                    if (methods[i].param_types[0] != '\0') {
                        printf("  Parameter Types: %s\n", methods[i].param_types);
                    }
                    if (methods[i].return_type[0] != '\0') {
                        printf("  Return Type: %s\n", methods[i].return_type);
                    } else {
                        printf("  Return Type: (procedure - no return)\n");
                    }
                    printf("  Flags: 0x%08X\n", methods[i].flags);
                    printf("\n");
                }
            }
            
            if (classes) {
                free(classes);
            }
            if (methods) {
                free(methods);
            }
            if (fields) {
                free(fields);
            }
        } else {
            printf("Error: Could not load classes section\n");
        }
    }
    
    if (show_hex) {
        printf("\n=== ARX Module Header (Hex) ===\n");
        printf("Offset  ");
        for (int i = 0; i < 16; i++) {
            printf("%02X ", i);
        }
        printf(" ASCII\n");
        printf("--------");
        for (int i = 0; i < 16; i++) {
            printf("---");
        }
        printf(" ------\n");
        
        // Read and display header as hex
        fseek(reader.file, 0, SEEK_SET);
        uint8_t buffer[ARXMOD_HEADER_SIZE];
        if (fread(buffer, 1, ARXMOD_HEADER_SIZE, reader.file) == ARXMOD_HEADER_SIZE) {
            for (int i = 0; i < ARXMOD_HEADER_SIZE; i += 16) {
                printf("%08X ", i);
                for (int j = 0; j < 16; j++) {
                    if (i + j < ARXMOD_HEADER_SIZE) {
                        printf("%02X ", buffer[i + j]);
                    } else {
                        printf("   ");
                    }
                }
                printf(" ");
                for (int j = 0; j < 16; j++) {
                    if (i + j < ARXMOD_HEADER_SIZE) {
                        char c = buffer[i + j];
                        printf("%c", (c >= 32 && c <= 126) ? c : '.');
                    }
                }
                printf("\n");
            }
        }
    }
    
    arxmod_reader_cleanup(&reader);
    
    return 0;
}
