/*
 * ARX Module Writer Implementation
 * Handles writing .arxmod files
 */

#include "arxmod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

bool arxmod_writer_init(arxmod_writer_t *writer, const char *filename)
{
    if (writer == NULL || filename == NULL) {
        return false;
    }
    
    writer->file = fopen(filename, "wb");
    if (writer->file == NULL) {
        return false;
    }
    
    writer->header_offset = 0;
    writer->toc_offset = 0;
    writer->data_offset = 0;
    writer->current_data_offset = 0;
    writer->section_count = 0;
    writer->toc_entries = NULL;
    writer->toc_capacity = 0;
    writer->debug_output = debug_mode;
    
    if (writer->debug_output) {
        printf("ARX module writer initialized for '%s'\n", filename);
    }
    
    return true;
}

bool arxmod_writer_write_header(arxmod_writer_t *writer, const char *app_name, size_t app_name_len)
{
    if (writer == NULL || writer->file == NULL) {
        return false;
    }
    
    // Initialize header
    arxmod_header_t header = {0};
    strncpy(header.magic, ARXMOD_MAGIC, 8);
    header.version = ARXMOD_VERSION;
    header.flags = 0;
    header.header_size = ARXMOD_HEADER_SIZE;
    header.toc_offset = 0; // Will be set later
    header.toc_size = 0;   // Will be set later
    header.data_offset = 0; // Will be set later
    header.data_size = 0;   // Will be set later
    header.app_name_len = app_name_len;
    header.app_data_size = 0; // Will be set later
    
    // Write header
    if (fwrite(&header, sizeof(arxmod_header_t), 1, writer->file) != 1) {
        return false;
    }
    
    writer->header_offset = 0;
    writer->toc_offset = ARXMOD_HEADER_SIZE;
    
    // Reserve space for TOC (will be written later)
    size_t toc_size = 8 * sizeof(arxmod_toc_entry_t); // Reserve space for 8 sections
    uint8_t *toc_placeholder = calloc(1, toc_size);
    if (toc_placeholder == NULL) {
        return false;
    }
    if (fwrite(toc_placeholder, 1, toc_size, writer->file) != toc_size) {
        free(toc_placeholder);
        return false;
    }
    free(toc_placeholder);
    
    writer->data_offset = ARXMOD_HEADER_SIZE + toc_size;
    
    if (writer->debug_output) {
        printf("ARX module header written\n");
    }
    
    return true;
}

bool arxmod_writer_add_code_section(arxmod_writer_t *writer, instruction_t *instructions, size_t instruction_count)
{
    if (writer == NULL || writer->file == NULL) {
        return false;
    }
    
    // Expand TOC if needed
    if (writer->section_count >= writer->toc_capacity) {
        size_t new_capacity = writer->toc_capacity == 0 ? 8 : writer->toc_capacity * 2;
        arxmod_toc_entry_t *new_toc = realloc(writer->toc_entries, new_capacity * sizeof(arxmod_toc_entry_t));
        if (new_toc == NULL) {
            return false;
        }
        writer->toc_entries = new_toc;
        writer->toc_capacity = new_capacity;
    }
    
    // Create TOC entry
    arxmod_toc_entry_t *toc_entry = &writer->toc_entries[writer->section_count];
    memset(toc_entry, 0, sizeof(arxmod_toc_entry_t));
    strncpy(toc_entry->section_name, ARXMOD_SECTION_CODE, 16);
    toc_entry->offset = writer->current_data_offset;
    toc_entry->size = instruction_count * sizeof(instruction_t);
    toc_entry->flags = 0;
    
    if (writer->debug_output) {
        printf("Created CODE TOC entry: name='%s', offset=%llu, size=%llu\n", 
               toc_entry->section_name, (unsigned long long)toc_entry->offset, (unsigned long long)toc_entry->size);
    }
    
    // Write section data
    if (instruction_count > 0) {
        // Ensure we're at the correct position in the file
        long expected_position = writer->data_offset + writer->current_data_offset;
        if (fseek(writer->file, expected_position, SEEK_SET) != 0) {
            return false;
        }
        
        if (writer->debug_output) {
            printf("Writing %zu instructions at file position %ld\n", instruction_count, expected_position);
            // Debug: print first few instructions
            for (size_t i = 0; i < instruction_count && i < 3; i++) {
                printf("  Instruction %zu: opcode=0x%02x, operand=0x%016llx\n", 
                       i, instructions[i].opcode, (unsigned long long)instructions[i].opt64);
            }
        }
        
        if (fwrite(instructions, sizeof(instruction_t), instruction_count, writer->file) != instruction_count) {
            return false;
        }
    }
    
    writer->current_data_offset += toc_entry->size;
    writer->section_count++;
    
    if (writer->debug_output) {
        printf("Code section added: %zu instructions (%llu bytes)\n", 
               instruction_count, (unsigned long long)toc_entry->size);
    }
    
    return true;
}

bool arxmod_writer_add_strings_section(arxmod_writer_t *writer, const char **strings, size_t string_count)
{
    if (writer == NULL || writer->file == NULL) {
        return false;
    }
    
    // Expand TOC if needed
    if (writer->section_count >= writer->toc_capacity) {
        size_t new_capacity = writer->toc_capacity == 0 ? 8 : writer->toc_capacity * 2;
        arxmod_toc_entry_t *new_toc = realloc(writer->toc_entries, new_capacity * sizeof(arxmod_toc_entry_t));
        if (new_toc == NULL) {
            return false;
        }
        writer->toc_entries = new_toc;
        writer->toc_capacity = new_capacity;
    }
    
    // Create TOC entry
    arxmod_toc_entry_t *toc_entry = &writer->toc_entries[writer->section_count];
    memset(toc_entry, 0, sizeof(arxmod_toc_entry_t));
    strncpy(toc_entry->section_name, ARXMOD_SECTION_STRINGS, 16);
    toc_entry->offset = writer->current_data_offset;
    toc_entry->flags = 0;
    
    // Calculate total size
    size_t total_size = 0;
    for (size_t i = 0; i < string_count; i++) {
        if (strings[i] != NULL) {
            total_size += strlen(strings[i]) + 1; // +1 for null terminator
        }
    }
    
    toc_entry->size = total_size;
    
    // Write string data
    for (size_t i = 0; i < string_count; i++) {
        if (strings[i] != NULL) {
            size_t len = strlen(strings[i]) + 1;
            if (fwrite(strings[i], 1, len, writer->file) != len) {
                return false;
            }
        }
    }
    
    writer->current_data_offset += toc_entry->size;
    writer->section_count++;
    
    if (writer->debug_output) {
        printf("Strings section added: %zu strings (%llu bytes)\n", 
               string_count, (unsigned long long)toc_entry->size);
    }
    
    return true;
}

bool arxmod_writer_add_symbols_section(arxmod_writer_t *writer, symbol_entry_t *symbols, size_t symbol_count)
{
    if (writer == NULL || writer->file == NULL) {
        return false;
    }
    
    // Expand TOC if needed
    if (writer->section_count >= writer->toc_capacity) {
        size_t new_capacity = writer->toc_capacity == 0 ? 8 : writer->toc_capacity * 2;
        arxmod_toc_entry_t *new_toc = realloc(writer->toc_entries, new_capacity * sizeof(arxmod_toc_entry_t));
        if (new_toc == NULL) {
            return false;
        }
        writer->toc_entries = new_toc;
        writer->toc_capacity = new_capacity;
    }
    
    // Create TOC entry
    arxmod_toc_entry_t *toc_entry = &writer->toc_entries[writer->section_count];
    memset(toc_entry, 0, sizeof(arxmod_toc_entry_t));
    strncpy(toc_entry->section_name, ARXMOD_SECTION_SYMBOLS, 16);
    toc_entry->offset = writer->current_data_offset;
    toc_entry->size = symbol_count * sizeof(symbol_entry_t);
    toc_entry->flags = 0;
    
    // Write symbol data
    if (symbol_count > 0) {
        if (fwrite(symbols, sizeof(symbol_entry_t), symbol_count, writer->file) != symbol_count) {
            return false;
        }
    }
    
    writer->current_data_offset += toc_entry->size;
    writer->section_count++;
    
    if (writer->debug_output) {
        printf("Symbols section added: %zu symbols (%llu bytes)\n", 
               symbol_count, (unsigned long long)toc_entry->size);
    }
    
    return true;
}

bool arxmod_writer_add_debug_section(arxmod_writer_t *writer, debug_entry_t *debug_info, size_t debug_count)
{
    if (writer == NULL || writer->file == NULL) {
        return false;
    }
    
    // Expand TOC if needed
    if (writer->section_count >= writer->toc_capacity) {
        size_t new_capacity = writer->toc_capacity == 0 ? 8 : writer->toc_capacity * 2;
        arxmod_toc_entry_t *new_toc = realloc(writer->toc_entries, new_capacity * sizeof(arxmod_toc_entry_t));
        if (new_toc == NULL) {
            return false;
        }
        writer->toc_entries = new_toc;
        writer->toc_capacity = new_capacity;
    }
    
    // Create TOC entry
    arxmod_toc_entry_t *toc_entry = &writer->toc_entries[writer->section_count];
    memset(toc_entry, 0, sizeof(arxmod_toc_entry_t));
    strncpy(toc_entry->section_name, ARXMOD_SECTION_DEBUG, 16);
    toc_entry->offset = writer->current_data_offset;
    toc_entry->size = debug_count * sizeof(debug_entry_t);
    toc_entry->flags = 0;
    
    // Write debug data
    if (debug_count > 0) {
        if (fwrite(debug_info, sizeof(debug_entry_t), debug_count, writer->file) != debug_count) {
            return false;
        }
    }
    
    writer->current_data_offset += toc_entry->size;
    writer->section_count++;
    
    if (writer->debug_output) {
        printf("Debug section added: %zu entries (%llu bytes)\n", 
               debug_count, (unsigned long long)toc_entry->size);
    }
    
    return true;
}

bool arxmod_writer_add_app_section(arxmod_writer_t *writer, const char *app_name, size_t app_name_len, const uint8_t *app_data, size_t app_data_size)
{
    if (writer == NULL || writer->file == NULL) {
        return false;
    }
    
    // Expand TOC if needed
    if (writer->section_count >= writer->toc_capacity) {
        size_t new_capacity = writer->toc_capacity == 0 ? 8 : writer->toc_capacity * 2;
        arxmod_toc_entry_t *new_toc = realloc(writer->toc_entries, new_capacity * sizeof(arxmod_toc_entry_t));
        if (new_toc == NULL) {
            return false;
        }
        writer->toc_entries = new_toc;
        writer->toc_capacity = new_capacity;
    }
    
    // Create TOC entry
    arxmod_toc_entry_t *toc_entry = &writer->toc_entries[writer->section_count];
    memset(toc_entry, 0, sizeof(arxmod_toc_entry_t));
    strncpy(toc_entry->section_name, ARXMOD_SECTION_APP, 16);
    toc_entry->offset = writer->current_data_offset;
    toc_entry->size = app_name_len + app_data_size;
    toc_entry->flags = 0;
    
    // Write app name
    if (app_name_len > 0) {
        if (fwrite(app_name, 1, app_name_len, writer->file) != app_name_len) {
            return false;
        }
    }
    
    // Write app data
    if (app_data_size > 0) {
        if (fwrite(app_data, 1, app_data_size, writer->file) != app_data_size) {
            return false;
        }
    }
    
    writer->current_data_offset += toc_entry->size;
    writer->section_count++;
    
    if (writer->debug_output) {
        printf("App section added: %llu bytes\n", (unsigned long long)toc_entry->size);
    }
    
    return true;
}

bool arxmod_writer_finalize(arxmod_writer_t *writer)
{
    if (writer == NULL || writer->file == NULL) {
        return false;
    }
    
    // Write TOC at the correct position
    fseek(writer->file, writer->toc_offset, SEEK_SET);
    if (writer->section_count > 0) {
        if (writer->debug_output) {
            printf("Writing TOC with %zu entries:\n", writer->section_count);
            for (size_t i = 0; i < writer->section_count; i++) {
                printf("  Entry %zu: name='%s', offset=%llu, size=%llu\n", 
                       i, writer->toc_entries[i].section_name, 
                       (unsigned long long)writer->toc_entries[i].offset, 
                       (unsigned long long)writer->toc_entries[i].size);
            }
        }
        if (fwrite(writer->toc_entries, sizeof(arxmod_toc_entry_t), writer->section_count, writer->file) != writer->section_count) {
            return false;
        }
        fflush(writer->file); // Ensure TOC is written to disk
    }
    
    // Update header with final values
    fseek(writer->file, writer->header_offset, SEEK_SET);
    
    arxmod_header_t header = {0};
    strncpy(header.magic, ARXMOD_MAGIC, 8);
    header.version = ARXMOD_VERSION;
    header.flags = 0;
    header.header_size = ARXMOD_HEADER_SIZE;
    header.toc_offset = writer->toc_offset;
    header.toc_size = writer->section_count * sizeof(arxmod_toc_entry_t);
    header.data_offset = writer->data_offset;
    header.data_size = writer->current_data_offset - writer->data_offset;
    header.app_name_len = 0; // Would be set from actual app data
    header.app_data_size = 0; // Would be set from actual app data
    
    if (fwrite(&header, sizeof(arxmod_header_t), 1, writer->file) != 1) {
        return false;
    }
    
    if (writer->debug_output) {
        printf("ARX module finalized: %zu sections, %zu bytes total\n", 
               writer->section_count, writer->current_data_offset);
    }
    
    return true;
}

void arxmod_writer_cleanup(arxmod_writer_t *writer)
{
    if (writer != NULL) {
        if (writer->file != NULL) {
            fclose(writer->file);
            writer->file = NULL;
        }
        if (writer->toc_entries != NULL) {
            free(writer->toc_entries);
            writer->toc_entries = NULL;
        }
        memset(writer, 0, sizeof(arxmod_writer_t));
    }
}
