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

bool arxmod_writer_set_flags(arxmod_writer_t *writer, uint32_t flags)
{
    if (writer == NULL) {
        return false;
    }
    
    writer->module_flags = flags;
    
    if (writer->debug_output) {
        printf("ARX module writer: Set flags to 0x%08x\n", flags);
    }
    
    return true;
}

bool arxmod_writer_set_entry_point(arxmod_writer_t *writer, uint64_t entry_point)
{
    if (writer == NULL) {
        return false;
    }
    
    writer->entry_point = entry_point;
    
    if (writer->debug_output) {
        printf("ARX module writer: Set entry point to 0x%llx\n", (unsigned long long)entry_point);
    }
    
    return true;
}

bool arxmod_writer_update_header(arxmod_writer_t *writer)
{
    if (writer == NULL || writer->file == NULL) {
        return false;
    }
    
    // Seek to the beginning of the file to update the header
    if (fseek(writer->file, 0, SEEK_SET) != 0) {
        if (writer->debug_output) {
            printf("ARX module writer: Failed to seek to beginning of file\n");
        }
        return false;
    }
    
    if (writer->debug_output) {
        long current_pos = ftell(writer->file);
        printf("ARX module writer: Seeking to position %ld to update header\n", current_pos);
    }
    
    // Verify we're at the beginning
    long verify_pos = ftell(writer->file);
    if (verify_pos != 0) {
        if (writer->debug_output) {
            printf("ARX module writer: Warning - not at beginning of file, position is %ld\n", verify_pos);
        }
    }
    
    // Create updated header
    arxmod_header_t header;
    memset(&header, 0, sizeof(arxmod_header_t));
    
    // Copy magic
    memcpy(header.magic, ARXMOD_MAGIC, 8);
    header.version = ARXMOD_VERSION;
    header.flags = writer->module_flags;
    header.header_size = ARXMOD_HEADER_SIZE;
    header.toc_offset = writer->toc_offset;
    header.toc_size = writer->section_count * sizeof(arxmod_toc_entry_t);
    header.data_offset = writer->data_offset;
    header.data_size = writer->current_data_offset;
    header.app_name_len = 10; // "ARXProgram"
    header.app_data_size = 0;
    header.entry_point = writer->entry_point;
    
    // Write updated header
    if (fwrite(&header, sizeof(arxmod_header_t), 1, writer->file) != 1) {
        if (writer->debug_output) {
            printf("ARX module writer: Failed to write updated header\n");
        }
        return false;
    }
    
    // Verify what was written
    if (writer->debug_output) {
        long after_write_pos = ftell(writer->file);
        printf("ARX module writer: After writing header, file position is %ld\n", after_write_pos);
    }
    
    if (writer->debug_output) {
        printf("ARX module writer: Updated header with entry point 0x%llx\n", (unsigned long long)writer->entry_point);
        printf("ARX module writer: Header size: %llu, Entry point: %llu\n", 
               (unsigned long long)header.header_size, (unsigned long long)header.entry_point);
    }
    
    // Flush the file to ensure changes are written to disk
    if (fflush(writer->file) != 0) {
        if (writer->debug_output) {
            printf("ARX module writer: Failed to flush file after header update\n");
        }
        return false;
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
    header.flags = writer->module_flags;
    header.header_size = ARXMOD_HEADER_SIZE;
    header.toc_offset = 0; // Will be set later
    header.toc_size = 0;   // Will be set later
    header.data_offset = 0; // Will be set later
    header.data_size = 0;   // Will be set later
    header.app_name_len = app_name_len;
    header.app_data_size = 0; // Will be set later
    header.entry_point = writer->entry_point;
    
    // Write header
    if (fwrite(&header, sizeof(arxmod_header_t), 1, writer->file) != 1) {
        return false;
    }
    
    writer->header_offset = 0;
    writer->toc_offset = ARXMOD_HEADER_SIZE;
    
    // Reserve space for TOC (will be written later)
    size_t toc_size = 6 * sizeof(arxmod_toc_entry_t); // Reserve space for 6 sections (CODE, STRINGS, SYMBOLS, DEBUG, CLASSES, APP)
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
        long expected_position = writer->data_offset + toc_entry->offset;
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
    
    // Seek to the correct position in the file
    long expected_position = writer->data_offset + toc_entry->offset;
    if (fseek(writer->file, expected_position, SEEK_SET) != 0) {
        return false;
    }
    
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
    
    if (writer->debug_output) {
        printf("SYMBOLS section: current_data_offset=%zu, offset=%zu, size=%zu\n", 
               writer->current_data_offset, toc_entry->offset, toc_entry->size);
    }
    
    // Seek to the correct position in the file
    long expected_position = writer->data_offset + toc_entry->offset;
    if (fseek(writer->file, expected_position, SEEK_SET) != 0) {
        return false;
    }
    
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
    
    if (writer->debug_output) {
        printf("DEBUG section: current_data_offset=%zu, offset=%zu, size=%zu\n", 
               writer->current_data_offset, toc_entry->offset, toc_entry->size);
    }
    
    // Seek to the correct position in the file
    long expected_position = writer->data_offset + toc_entry->offset;
    if (fseek(writer->file, expected_position, SEEK_SET) != 0) {
        return false;
    }
    
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

bool arxmod_writer_add_classes_section(arxmod_writer_t *writer, class_entry_t *classes, size_t class_count, method_entry_t *methods, size_t method_count, field_entry_t *fields, size_t field_count)
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
    strncpy(toc_entry->section_name, ARXMOD_SECTION_CLASSES, 16);
    toc_entry->offset = writer->current_data_offset; // This is the offset within the data section
    // Size includes classes plus inline methods and fields
    toc_entry->size = class_count * sizeof(class_entry_t) + method_count * sizeof(method_entry_t) + field_count * sizeof(field_entry_t);
    toc_entry->flags = 0;
    
    if (writer->debug_output) {
        printf("CLASSES section: current_data_offset=%zu, offset=%zu, size=%zu\n", 
               writer->current_data_offset, toc_entry->offset, toc_entry->size);
    }
    
    // Seek to the correct position in the file
    long expected_position = writer->data_offset + toc_entry->offset;
    if (writer->debug_output) {
        printf("CLASSES: Seeking to position %ld (data_offset=%zu + offset=%zu)\n", 
               expected_position, writer->data_offset, toc_entry->offset);
    }
    if (fseek(writer->file, expected_position, SEEK_SET) != 0) {
        if (writer->debug_output) {
            printf("CLASSES: fseek failed!\n");
        }
        return false;
    }
    if (writer->debug_output) {
        long actual_position = ftell(writer->file);
        printf("CLASSES: Actual position after fseek: %ld\n", actual_position);
    }
    
    // Write class data with inline methods and fields
    if (class_count > 0) {
        if (writer->debug_output) {
            printf("Writing %zu classes to offset %ld:\n", class_count, expected_position);
            for (size_t i = 0; i < class_count; i++) {
                printf("  Class %zu: name='%s', id=%llu, fields=%u, methods=%u\n", 
                       i, classes[i].class_name, (unsigned long long)classes[i].class_id, 
                       classes[i].field_count, classes[i].method_count);
            }
        }
        
        // Write classes with inline methods and fields
        size_t method_index = 0;
        size_t field_index = 0;
        
        for (size_t i = 0; i < class_count; i++) {
            // Write class entry
            if (fwrite(&classes[i], sizeof(class_entry_t), 1, writer->file) != 1) {
                return false;
            }
            
            // Write inline methods for this class
            for (size_t j = 0; j < classes[i].method_count; j++) {
                if (method_index < method_count) {
                    if (fwrite(&methods[method_index], sizeof(method_entry_t), 1, writer->file) != 1) {
                        return false;
                    }
                    method_index++;
                }
            }
            
            // Write inline fields for this class
            for (size_t j = 0; j < classes[i].field_count; j++) {
                if (field_index < field_count) {
                    if (fwrite(&fields[field_index], sizeof(field_entry_t), 1, writer->file) != 1) {
                        return false;
                    }
                    field_index++;
                }
            }
        }
    }
    
    writer->current_data_offset += toc_entry->size;
    writer->section_count++;
    
    if (writer->debug_output) {
        printf("Classes section added: %zu classes (%llu bytes)\n", 
               class_count, (unsigned long long)toc_entry->size);
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
    
    // Seek to the correct position in the file
    long expected_position = writer->data_offset + toc_entry->offset;
    if (fseek(writer->file, expected_position, SEEK_SET) != 0) {
        return false;
    }
    
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
    header.flags = writer->module_flags;
    header.header_size = ARXMOD_HEADER_SIZE;
    header.toc_offset = writer->toc_offset;
    header.toc_size = writer->section_count * sizeof(arxmod_toc_entry_t);
    header.data_offset = writer->data_offset; // Use the actual data_offset used when writing sections
    header.data_size = writer->current_data_offset;
    
    // Don't update writer->data_offset here - it's already correct from when sections were written
    header.app_name_len = 0; // Would be set from actual app data
    header.app_data_size = 0; // Would be set from actual app data
    
    if (writer->debug_output) {
        printf("ARX module writer: data_offset=%zu, current_data_offset=%zu, data_size=%llu\n",
               writer->data_offset, writer->current_data_offset, (unsigned long long)header.data_size);
    }
    
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
