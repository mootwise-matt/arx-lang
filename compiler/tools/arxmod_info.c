/*
 * ARX Module Information Tool
 * Displays information about .arxmod files
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../arxmod/arxmod.h"

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
    printf("  -validate      Validate file format\n");
    printf("  -hex           Show hex dump of header\n");
    printf("  -h, --help     Show this help message\n");
    printf("\n");
    printf("Examples:\n");
    printf("  %s module.arxmod\n", program_name);
    printf("  %s -sections module.arxmod\n", program_name);
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
        uint8_t buffer[64];
        if (fread(buffer, 1, 64, reader.file) == 64) {
            for (int i = 0; i < 64; i += 16) {
                printf("%08X ", i);
                for (int j = 0; j < 16; j++) {
                    if (i + j < 64) {
                        printf("%02X ", buffer[i + j]);
                    } else {
                        printf("   ");
                    }
                }
                printf(" ");
                for (int j = 0; j < 16; j++) {
                    if (i + j < 64) {
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
