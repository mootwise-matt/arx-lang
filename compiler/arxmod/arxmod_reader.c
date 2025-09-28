/*
 * ARX Module Reader Implementation
 * Handles reading .arxmod files
 */

#include "arxmod.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

bool arxmod_reader_init(arxmod_reader_t *reader, const char *filename)
{
    if (reader == NULL || filename == NULL) {
        return false;
    }
    
    reader->file = fopen(filename, "rb");
    if (reader->file == NULL) {
        return false;
    }
    
    reader->toc = NULL;
    reader->toc_count = 0;
    reader->debug_output = debug_mode;
    
    if (reader->debug_output) {
        printf("ARX module reader initialized for '%s'\n", filename);
    }
    
    return true;
}

bool arxmod_reader_validate(arxmod_reader_t *reader)
{
    if (reader == NULL || reader->file == NULL) {
        return false;
    }
    
    // Read header
    if (fread(&reader->header, sizeof(arxmod_header_t), 1, reader->file) != 1) {
        return false;
    }
    
    if (reader->debug_output) {
        printf("ARX module reader: Read header, data_size=%llu\n", 
               (unsigned long long)reader->header.data_size);
    }
    
    // Validate magic number
    if (strncmp(reader->header.magic, ARXMOD_MAGIC, 8) != 0) {
        if (reader->debug_output) {
            printf("Error: Invalid magic number\n");
        }
        return false;
    }
    
    // Validate version
    if (reader->header.version != ARXMOD_VERSION) {
        if (reader->debug_output) {
            printf("Error: Unsupported version %u (expected %d)\n", 
                   reader->header.version, ARXMOD_VERSION);
        }
        return false;
    }
    
    // Validate header size
    if (reader->header.header_size != ARXMOD_HEADER_SIZE) {
        if (reader->debug_output) {
            printf("Error: Invalid header size %llu (expected %d)\n", 
                   (unsigned long long)reader->header.header_size, ARXMOD_HEADER_SIZE);
        }
        return false;
    }
    
    if (reader->debug_output) {
        printf("ARX module validation successful\n");
        printf("  Version: %u\n", reader->header.version);
        printf("  TOC offset: %llu\n", (unsigned long long)reader->header.toc_offset);
        printf("  TOC size: %llu\n", (unsigned long long)reader->header.toc_size);
        printf("  Data offset: %llu\n", (unsigned long long)reader->header.data_offset);
        printf("  Data size: %llu\n", (unsigned long long)reader->header.data_size);
    }
    
    return true;
}

bool arxmod_reader_load_toc(arxmod_reader_t *reader)
{
    if (reader == NULL || reader->file == NULL) {
        return false;
    }
    
    // Seek to TOC
    if (fseek(reader->file, reader->header.toc_offset, SEEK_SET) != 0) {
        return false;
    }
    
    // Calculate number of TOC entries
    reader->toc_count = reader->header.toc_size / sizeof(arxmod_toc_entry_t);
    
    // Allocate TOC
    reader->toc = malloc(reader->toc_count * sizeof(arxmod_toc_entry_t));
    if (reader->toc == NULL) {
        return false;
    }
    
    // Read TOC
    if (fread(reader->toc, sizeof(arxmod_toc_entry_t), reader->toc_count, reader->file) != reader->toc_count) {
        free(reader->toc);
        reader->toc = NULL;
        return false;
    }
    
    if (reader->debug_output) {
        printf("TOC loaded: %zu sections\n", reader->toc_count);
        for (size_t i = 0; i < reader->toc_count; i++) {
            printf("  Section %zu: %s (offset: %llu, size: %llu)\n", 
                   i, reader->toc[i].section_name,
                   (unsigned long long)reader->toc[i].offset,
                   (unsigned long long)reader->toc[i].size);
        }
    }
    
    return true;
}

arxmod_toc_entry_t* arxmod_reader_find_section(arxmod_reader_t *reader, const char *section_name)
{
    if (reader == NULL || reader->toc == NULL || section_name == NULL) {
        return NULL;
    }
    
    for (size_t i = 0; i < reader->toc_count; i++) {
        if (strcmp(reader->toc[i].section_name, section_name) == 0) {
            return &reader->toc[i];
        }
    }
    
    return NULL;
}

bool arxmod_reader_load_code_section(arxmod_reader_t *reader, instruction_t **instructions, size_t *instruction_count)
{
    if (reader == NULL || reader->file == NULL || instructions == NULL || instruction_count == NULL) {
        return false;
    }
    
    arxmod_toc_entry_t *section = arxmod_reader_find_section(reader, ARXMOD_SECTION_CODE);
    if (section == NULL) {
        // If no "CODE" section found, try the first section (for backward compatibility)
        if (reader->toc_count > 0) {
            section = &reader->toc[0];
            if (section->size == 0) {
                *instructions = NULL;
                *instruction_count = 0;
                return true; // No code section is valid
            }
        } else {
            *instructions = NULL;
            *instruction_count = 0;
            return true; // No code section is valid
        }
    }
    
    // Calculate number of instructions
    *instruction_count = section->size / sizeof(instruction_t);
    
    // Allocate instructions
    *instructions = malloc(section->size);
    if (*instructions == NULL) {
        return false;
    }
    
    // Seek to section data
    size_t seek_offset = reader->header.data_offset + section->offset;
    if (reader->debug_output) {
        printf("Seeking to offset: %zu (data_offset=%llu + section_offset=%llu)\n", 
               seek_offset, (unsigned long long)reader->header.data_offset, (unsigned long long)section->offset);
    }
    
    if (fseek(reader->file, seek_offset, SEEK_SET) != 0) {
        free(*instructions);
        *instructions = NULL;
        return false;
    }
    
    // Read instructions
    if (fread(*instructions, sizeof(instruction_t), *instruction_count, reader->file) != *instruction_count) {
        free(*instructions);
        *instructions = NULL;
        return false;
    }
    
    if (reader->debug_output) {
        printf("Read %zu instructions from offset %zu\n", *instruction_count, seek_offset);
        printf("Instruction structure size: %zu bytes\n", sizeof(instruction_t));
        for (size_t i = 0; i < *instruction_count && i < 5; i++) {
            instruction_t *instr = &(*instructions)[i];
            printf("  Instruction %zu: raw_opcode=0x%02x, opcode=%d, operand=%llu\n", 
                   i, instr->opcode, instr->opcode & 0xF, (unsigned long long)instr->opt64);
        }
    }
    
    if (reader->debug_output) {
        printf("Code section loaded: %zu instructions\n", *instruction_count);
    }
    
    return true;
}

bool arxmod_reader_load_strings_section(arxmod_reader_t *reader, char ***strings, size_t *string_count)
{
    if (reader == NULL || reader->file == NULL || strings == NULL || string_count == NULL) {
        return false;
    }
    
    arxmod_toc_entry_t *section = arxmod_reader_find_section(reader, ARXMOD_SECTION_STRINGS);
    if (section == NULL) {
        *strings = NULL;
        *string_count = 0;
        return true; // No strings section is valid
    }
    
    if (section->size == 0) {
        *strings = NULL;
        *string_count = 0;
        return true;
    }
    
    // Seek to the strings section
    size_t seek_offset = reader->header.data_offset + section->offset;
    if (reader->debug_output) {
        printf("Seeking to strings section at offset: %zu (data_offset=%llu + section_offset=%llu)\n", 
               seek_offset, (unsigned long long)reader->header.data_offset, (unsigned long long)section->offset);
    }
    if (fseek(reader->file, seek_offset, SEEK_SET) != 0) {
        return false;
    }
    
    // Read the string data
    char *string_data = malloc(section->size);
    if (string_data == NULL) {
        return false;
    }
    
    if (fread(string_data, 1, section->size, reader->file) != section->size) {
        free(string_data);
        return false;
    }
    
    // Count the number of strings (null-terminated strings)
    size_t count = 0;
    for (size_t i = 0; i < section->size; i++) {
        if (string_data[i] == '\0') {
            count++;
        }
    }
    
    if (count == 0) {
        free(string_data);
        *strings = NULL;
        *string_count = 0;
        return true;
    }
    
    // Allocate array of string pointers
    char **string_array = malloc(sizeof(char*) * count);
    if (string_array == NULL) {
        free(string_data);
        return false;
    }
    
    // Parse strings
    size_t string_index = 0;
    size_t start = 0;
    for (size_t i = 0; i < section->size && string_index < count; i++) {
        if (string_data[i] == '\0') {
            size_t len = i - start;
            string_array[string_index] = malloc(len + 1);
            if (string_array[string_index] != NULL) {
                strncpy(string_array[string_index], &string_data[start], len);
                string_array[string_index][len] = '\0';
            }
            string_index++;
            start = i + 1;
        }
    }
    
    free(string_data);
    
    *strings = string_array;
    *string_count = count;
    
    if (reader->debug_output) {
        printf("Strings section loaded: %zu strings (%llu bytes)\n", 
               count, (unsigned long long)section->size);
    }
    
    return true;
}

bool arxmod_reader_load_symbols_section(arxmod_reader_t *reader, symbol_entry_t **symbols, size_t *symbol_count)
{
    if (reader == NULL || reader->file == NULL || symbols == NULL || symbol_count == NULL) {
        return false;
    }
    
    arxmod_toc_entry_t *section = arxmod_reader_find_section(reader, ARXMOD_SECTION_SYMBOLS);
    if (section == NULL) {
        *symbols = NULL;
        *symbol_count = 0;
        return true; // No symbols section is valid
    }
    
    // Calculate number of symbols
    *symbol_count = section->size / sizeof(symbol_entry_t);
    
    // Allocate symbols
    *symbols = malloc(section->size);
    if (*symbols == NULL) {
        return false;
    }
    
    // Seek to section data
    if (fseek(reader->file, reader->header.data_offset + section->offset, SEEK_SET) != 0) {
        free(*symbols);
        *symbols = NULL;
        return false;
    }
    
    // Read symbols
    if (fread(*symbols, sizeof(symbol_entry_t), *symbol_count, reader->file) != *symbol_count) {
        free(*symbols);
        *symbols = NULL;
        return false;
    }
    
    if (reader->debug_output) {
        printf("Symbols section loaded: %zu symbols\n", *symbol_count);
    }
    
    return true;
}

bool arxmod_reader_load_debug_section(arxmod_reader_t *reader, debug_entry_t **debug_info, size_t *debug_count)
{
    if (reader == NULL || reader->file == NULL || debug_info == NULL || debug_count == NULL) {
        return false;
    }
    
    arxmod_toc_entry_t *section = arxmod_reader_find_section(reader, ARXMOD_SECTION_DEBUG);
    if (section == NULL) {
        *debug_info = NULL;
        *debug_count = 0;
        return true; // No debug section is valid
    }
    
    // Calculate number of debug entries
    *debug_count = section->size / sizeof(debug_entry_t);
    
    // Allocate debug info
    *debug_info = malloc(section->size);
    if (*debug_info == NULL) {
        return false;
    }
    
    // Seek to section data
    if (fseek(reader->file, reader->header.data_offset + section->offset, SEEK_SET) != 0) {
        free(*debug_info);
        *debug_info = NULL;
        return false;
    }
    
    // Read debug info
    if (fread(*debug_info, sizeof(debug_entry_t), *debug_count, reader->file) != *debug_count) {
        free(*debug_info);
        *debug_info = NULL;
        return false;
    }
    
    if (reader->debug_output) {
        printf("Debug section loaded: %zu entries\n", *debug_count);
    }
    
    return true;
}

bool arxmod_reader_load_classes_section(arxmod_reader_t *reader, class_entry_t **classes, size_t *class_count, method_entry_t **methods, size_t *method_count, field_entry_t **fields, size_t *field_count)
{
    if (reader == NULL || reader->file == NULL || classes == NULL || class_count == NULL) {
        return false;
    }
    
    arxmod_toc_entry_t *section = arxmod_reader_find_section(reader, ARXMOD_SECTION_CLASSES);
    if (section == NULL) {
        *classes = NULL;
        *class_count = 0;
        return true; // No classes section is valid
    }
    
    // Seek to section data
    if (fseek(reader->file, reader->header.data_offset + section->offset, SEEK_SET) != 0) {
        return false;
    }
    
    if (reader->debug_output) {
        printf("DEBUG: Loading classes section, size=%llu bytes, offset=%llu\n", 
               (unsigned long long)section->size, (unsigned long long)section->offset);
        printf("DEBUG: Data section starts at %llu, classes section at %llu\n",
               (unsigned long long)reader->header.data_offset, 
               (unsigned long long)(reader->header.data_offset + section->offset));
    }
    
    // For now, let's use the old approach but fix the class count issue
    // TODO: Implement proper inline methods and fields storage
    
    // First pass: read all class entries to determine counts
    size_t remaining_size = section->size;
    *class_count = 0;
    size_t total_method_count = 0;
    size_t total_field_count = 0;
    
    // Count classes by reading them one by one
    while (remaining_size >= sizeof(class_entry_t)) {
        class_entry_t temp_class;
        if (fread(&temp_class, sizeof(class_entry_t), 1, reader->file) != 1) {
            if (reader->debug_output) {
                printf("DEBUG: Failed to read class entry, remaining_size=%zu\n", remaining_size);
            }
            break;
        }
        
        (*class_count)++;
        total_method_count += temp_class.method_count;
        total_field_count += temp_class.field_count;
        
        if (reader->debug_output) {
            printf("DEBUG: First pass - class %zu: name='%s', methods=%u, fields=%u\n", 
                   *class_count, temp_class.class_name, temp_class.method_count, temp_class.field_count);
        }
        
        remaining_size -= sizeof(class_entry_t);
        
        // Skip over inline methods and fields for this class
        size_t methods_size = temp_class.method_count * sizeof(method_entry_t);
        size_t fields_size = temp_class.field_count * sizeof(field_entry_t);
        size_t skip_size = methods_size + fields_size;
        
        if (skip_size > 0) {
            if (fseek(reader->file, skip_size, SEEK_CUR) != 0) {
                break;
            }
            remaining_size -= skip_size;
        }
        
        if (reader->debug_output) {
            printf("DEBUG: Read class %zu: name='%s', id=%llu, fields=%u, methods=%u\n", 
                   *class_count, temp_class.class_name, (unsigned long long)temp_class.class_id, 
                   temp_class.field_count, temp_class.method_count);
        }
    }
    
    if (reader->debug_output) {
        printf("DEBUG: Total counts: %zu classes, %zu methods, %zu fields\n", 
               *class_count, total_method_count, total_field_count);
    }
    
    // Allocate memory for classes, methods, and fields
    if (*class_count > 0) {
        *classes = malloc(*class_count * sizeof(class_entry_t));
        if (*classes == NULL) return false;
    } else {
        *classes = NULL;
    }
    
    if (total_method_count > 0) {
        *methods = malloc(total_method_count * sizeof(method_entry_t));
        if (*methods == NULL) {
            if (*classes) free(*classes);
            return false;
        }
    } else {
        *methods = NULL;
    }
    
    if (total_field_count > 0) {
        *fields = malloc(total_field_count * sizeof(field_entry_t));
        if (*fields == NULL) {
            if (*classes) free(*classes);
            if (*methods) free(*methods);
            return false;
        }
    } else {
        *fields = NULL;
    }
    
    // Set the counts
    *method_count = total_method_count;
    *field_count = total_field_count;
    
    // Second pass: seek back to beginning and read all class data
    if (fseek(reader->file, reader->header.data_offset + section->offset, SEEK_SET) != 0) {
        if (*classes) free(*classes);
        if (*methods) free(*methods);
        if (*fields) free(*fields);
        return false;
    }
    
    // Read the class data and inline methods/fields
    size_t method_index = 0;
    size_t field_index = 0;
    
    for (size_t i = 0; i < *class_count; i++) {
        // Read class entry
        if (fread(&(*classes)[i], sizeof(class_entry_t), 1, reader->file) != 1) {
            if (*classes) free(*classes);
            if (*methods) free(*methods);
            if (*fields) free(*fields);
            return false;
        }
        
        // Read inline methods for this class
        for (size_t j = 0; j < (*classes)[i].method_count; j++) {
            if (fread(&(*methods)[method_index], sizeof(method_entry_t), 1, reader->file) != 1) {
                if (*classes) free(*classes);
                if (*methods) free(*methods);
                if (*fields) free(*fields);
                return false;
            }
            method_index++;
        }
        
        // Read inline fields for this class
        for (size_t j = 0; j < (*classes)[i].field_count; j++) {
            if (fread(&(*fields)[field_index], sizeof(field_entry_t), 1, reader->file) != 1) {
                if (*classes) free(*classes);
                if (*methods) free(*methods);
                if (*fields) free(*fields);
                return false;
            }
            field_index++;
        }
    }
    
    if (reader->debug_output) {
        printf("Read %zu classes from offset %llu:\n", 
               *class_count, (unsigned long long)(reader->header.data_offset + section->offset));
        for (size_t i = 0; i < *class_count; i++) {
            printf("  Class %zu: name='%s', id=%llu, fields=%u, methods=%u\n", 
                   i, (*classes)[i].class_name, (unsigned long long)(*classes)[i].class_id, 
                   (*classes)[i].field_count, (*classes)[i].method_count);
        }
    }
    
    return true;
}


bool arxmod_reader_load_app_section(arxmod_reader_t *reader, char **app_name, uint8_t **app_data, size_t *app_data_size)
{
    if (reader == NULL || reader->file == NULL || app_name == NULL || app_data == NULL || app_data_size == NULL) {
        return false;
    }
    
    arxmod_toc_entry_t *section = arxmod_reader_find_section(reader, ARXMOD_SECTION_APP);
    if (section == NULL) {
        *app_name = NULL;
        *app_data = NULL;
        *app_data_size = 0;
        return true; // No app section is valid
    }
    
    // Seek to section data
    if (fseek(reader->file, reader->header.data_offset + section->offset, SEEK_SET) != 0) {
        return false;
    }
    
    // Read app name
    if (reader->header.app_name_len > 0) {
        *app_name = malloc(reader->header.app_name_len + 1);
        if (*app_name == NULL) {
            return false;
        }
        
        if (fread(*app_name, 1, reader->header.app_name_len, reader->file) != reader->header.app_name_len) {
            free(*app_name);
            *app_name = NULL;
            return false;
        }
        
        (*app_name)[reader->header.app_name_len] = '\0';
    } else {
        *app_name = NULL;
    }
    
    // Read app data
    *app_data_size = reader->header.app_data_size;
    if (*app_data_size > 0) {
        *app_data = malloc(*app_data_size);
        if (*app_data == NULL) {
            free(*app_name);
            *app_name = NULL;
            return false;
        }
        
        if (fread(*app_data, 1, *app_data_size, reader->file) != *app_data_size) {
            free(*app_name);
            free(*app_data);
            *app_name = NULL;
            *app_data = NULL;
            return false;
        }
    } else {
        *app_data = NULL;
    }
    
    if (reader->debug_output) {
        printf("App section loaded: name='%s', data=%zu bytes\n", 
               *app_name ? *app_name : "(none)", *app_data_size);
    }
    
    return true;
}

void arxmod_reader_cleanup(arxmod_reader_t *reader)
{
    if (reader != NULL) {
        if (reader->file != NULL) {
            fclose(reader->file);
            reader->file = NULL;
        }
        if (reader->toc != NULL) {
            free(reader->toc);
            reader->toc = NULL;
        }
        memset(reader, 0, sizeof(arxmod_reader_t));
    }
}

void arxmod_dump_info(arxmod_reader_t *reader)
{
    if (reader == NULL) {
        printf("ARX module reader is NULL\n");
        return;
    }
    
    printf("\n=== ARX Module Information ===\n");
    printf("Magic: %.8s\n", reader->header.magic);
    printf("Version: %u\n", reader->header.version);
    printf("Flags: 0x%08x\n", reader->header.flags);
    printf("Header size: %llu bytes\n", (unsigned long long)reader->header.header_size);
    printf("TOC offset: %llu\n", (unsigned long long)reader->header.toc_offset);
    printf("TOC size: %llu bytes\n", (unsigned long long)reader->header.toc_size);
    printf("Data offset: %llu\n", (unsigned long long)reader->header.data_offset);
    printf("Data size: %llu bytes\n", (unsigned long long)reader->header.data_size);
    printf("App name length: %llu\n", (unsigned long long)reader->header.app_name_len);
    printf("App data size: %llu bytes\n", (unsigned long long)reader->header.app_data_size);
    printf("Total file size: %llu bytes\n", 
           (unsigned long long)(reader->header.data_offset + reader->header.data_size));
    printf("\n");
}

void arxmod_dump_sections(arxmod_reader_t *reader)
{
    if (reader == NULL || reader->toc == NULL) {
        printf("No sections to dump\n");
        return;
    }
    
    printf("\n=== ARX Module Sections ===\n");
    for (size_t i = 0; i < reader->toc_count; i++) {
        printf("Section %zu: %s\n", i, reader->toc[i].section_name);
        printf("  Offset: %llu\n", (unsigned long long)reader->toc[i].offset);
        printf("  Size: %llu bytes\n", (unsigned long long)reader->toc[i].size);
        printf("  Flags: 0x%08x\n", reader->toc[i].flags);
        printf("\n");
    }
}

bool arxmod_validate_file(const char *filename)
{
    arxmod_reader_t reader;
    
    if (!arxmod_reader_init(&reader, filename)) {
        return false;
    }
    
    bool valid = arxmod_reader_validate(&reader);
    
    arxmod_reader_cleanup(&reader);
    
    return valid;
}

uint64_t arxmod_calculate_hash(const uint8_t *data, size_t length)
{
    uint64_t hash = 5381;
    for (size_t i = 0; i < length; i++) {
        hash = ((hash << 5) + hash) + data[i];
    }
    return hash;
}
