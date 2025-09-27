/*
 * ARX Type System
 * Defines types, type checking, and type operations
 */

#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

// Type categories
typedef enum {
    TYPE_CATEGORY_NONE = 0,
    TYPE_CATEGORY_PRIMITIVE,
    TYPE_CATEGORY_OBJECT,
    TYPE_CATEGORY_ARRAY,
    TYPE_CATEGORY_CLASS
} type_category_t;

// Primitive types
typedef enum {
    TYPE_NONE = 0,
    TYPE_INTEGER,
    TYPE_BOOLEAN,
    TYPE_CHAR,
    TYPE_REAL
} primitive_type_t;

// Object types
typedef enum {
    OBJ_TYPE_NONE = 0,
    OBJ_TYPE_STRING,
    OBJ_TYPE_ARRAY,
    OBJ_TYPE_CLASS
} object_type_t;

// Type information structure
typedef struct type_info {
    type_category_t category;
    union {
        primitive_type_t primitive;
        object_type_t object;
        struct {
            char *class_name;        // For class types
            size_t class_name_len;
        } class_info;
        struct {
            struct type_info *element_type;  // For array types
            size_t array_size;               // 0 for dynamic arrays
        } array_info;
    } data;
    size_t size;                    // Size in bytes
    bool is_const;                  // Is constant type
    bool is_reference;              // Is reference type
} type_info_t;

// Type system initialization
void types_init(void);
void types_cleanup(void);

// Type system functions
type_info_t* type_create_primitive(primitive_type_t primitive);
type_info_t* type_create_object(object_type_t object);
type_info_t* type_create_class(const char *class_name, size_t class_name_len);
type_info_t* type_create_array(type_info_t *element_type, size_t array_size);
type_info_t* type_create_reference(type_info_t *base_type);
type_info_t* type_create_const(type_info_t *base_type);

void type_destroy(type_info_t *type);
type_info_t* type_copy(const type_info_t *type);

// Type checking functions
bool type_is_primitive(const type_info_t *type);
bool type_is_object(const type_info_t *type);
bool type_is_array(const type_info_t *type);
bool type_is_class(const type_info_t *type);
bool type_is_reference(const type_info_t *type);
bool type_is_const(const type_info_t *type);

bool type_equals(const type_info_t *type1, const type_info_t *type2);
bool type_compatible(const type_info_t *from, const type_info_t *to);
bool type_assignable(const type_info_t *from, const type_info_t *to);

// Type conversion functions
type_info_t* type_promote(const type_info_t *type);
type_info_t* type_demote(const type_info_t *type);

// Utility functions
const char* type_to_string(const type_info_t *type);
size_t type_get_size(const type_info_t *type);
bool type_is_numeric(const type_info_t *type);
bool type_is_integral(const type_info_t *type);
bool type_is_floating(const type_info_t *type);

// Predefined types
extern type_info_t *TYPE_INTEGER_PREDEF;
extern type_info_t *TYPE_BOOLEAN_PREDEF;
extern type_info_t *TYPE_CHAR_PREDEF;
extern type_info_t *TYPE_REAL_PREDEF;
extern type_info_t *TYPE_STRING_PREDEF;
