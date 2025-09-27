/*
 * ARX Type System Implementation
 * Implements type checking, type operations, and type management
 */

#include "types.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global debug flag (extern from main.c)
extern bool debug_mode;

// Predefined types
type_info_t *TYPE_INTEGER_PREDEF = NULL;
type_info_t *TYPE_BOOLEAN_PREDEF = NULL;
type_info_t *TYPE_CHAR_PREDEF = NULL;
type_info_t *TYPE_REAL_PREDEF = NULL;
type_info_t *TYPE_STRING_PREDEF = NULL;

// Initialize predefined types
void types_init(void)
{
    if (TYPE_INTEGER_PREDEF == NULL) {
        TYPE_INTEGER_PREDEF = type_create_primitive(TYPE_INTEGER);
        TYPE_BOOLEAN_PREDEF = type_create_primitive(TYPE_BOOLEAN);
        TYPE_CHAR_PREDEF = type_create_primitive(TYPE_CHAR);
        TYPE_REAL_PREDEF = type_create_primitive(TYPE_REAL);
        TYPE_STRING_PREDEF = type_create_object(OBJ_TYPE_STRING);
    }
}

// Cleanup predefined types
void types_cleanup(void)
{
    if (TYPE_INTEGER_PREDEF != NULL) {
        type_destroy(TYPE_INTEGER_PREDEF);
        type_destroy(TYPE_BOOLEAN_PREDEF);
        type_destroy(TYPE_CHAR_PREDEF);
        type_destroy(TYPE_REAL_PREDEF);
        type_destroy(TYPE_STRING_PREDEF);
        TYPE_INTEGER_PREDEF = NULL;
        TYPE_BOOLEAN_PREDEF = NULL;
        TYPE_CHAR_PREDEF = NULL;
        TYPE_REAL_PREDEF = NULL;
        TYPE_STRING_PREDEF = NULL;
    }
}

type_info_t* type_create_primitive(primitive_type_t primitive)
{
    type_info_t *type = malloc(sizeof(type_info_t));
    if (type == NULL) {
        return NULL;
    }
    
    type->category = TYPE_CATEGORY_PRIMITIVE;
    type->data.primitive = primitive;
    type->is_const = false;
    type->is_reference = false;
    
    // Set size based on primitive type
    switch (primitive) {
        case TYPE_INTEGER:
            type->size = sizeof(int64_t);
            break;
        case TYPE_BOOLEAN:
            type->size = sizeof(bool);
            break;
        case TYPE_CHAR:
            type->size = sizeof(char);
            break;
        case TYPE_REAL:
            type->size = sizeof(double);
            break;
        default:
            type->size = 0;
            break;
    }
    
    return type;
}

type_info_t* type_create_object(object_type_t object)
{
    type_info_t *type = malloc(sizeof(type_info_t));
    if (type == NULL) {
        return NULL;
    }
    
    type->category = TYPE_CATEGORY_OBJECT;
    type->data.object = object;
    type->is_const = false;
    type->is_reference = false;
    
    // Set size based on object type
    switch (object) {
        case OBJ_TYPE_STRING:
            type->size = sizeof(void*); // String is a reference
            type->is_reference = true;
            break;
        case OBJ_TYPE_ARRAY:
            type->size = sizeof(void*); // Array is a reference
            type->is_reference = true;
            break;
        default:
            type->size = 0;
            break;
    }
    
    return type;
}

type_info_t* type_create_class(const char *class_name, size_t class_name_len)
{
    type_info_t *type = malloc(sizeof(type_info_t));
    if (type == NULL) {
        return NULL;
    }
    
    type->category = TYPE_CATEGORY_CLASS;
    type->data.class_info.class_name = malloc(class_name_len + 1);
    if (type->data.class_info.class_name == NULL) {
        free(type);
        return NULL;
    }
    
    strncpy(type->data.class_info.class_name, class_name, class_name_len);
    type->data.class_info.class_name[class_name_len] = '\0';
    type->data.class_info.class_name_len = class_name_len;
    
    type->is_const = false;
    type->is_reference = true; // Classes are reference types
    type->size = sizeof(void*); // Class instances are references
    
    return type;
}

type_info_t* type_create_array(type_info_t *element_type, size_t array_size)
{
    type_info_t *type = malloc(sizeof(type_info_t));
    if (type == NULL) {
        return NULL;
    }
    
    type->category = TYPE_CATEGORY_ARRAY;
    type->data.array_info.element_type = type_copy(element_type);
    type->data.array_info.array_size = array_size;
    type->is_const = false;
    type->is_reference = true; // Arrays are reference types
    type->size = sizeof(void*); // Array references
    
    return type;
}

type_info_t* type_create_reference(type_info_t *base_type)
{
    type_info_t *type = type_copy(base_type);
    if (type == NULL) {
        return NULL;
    }
    
    type->is_reference = true;
    type->size = sizeof(void*); // References are pointers
    
    return type;
}

type_info_t* type_create_const(type_info_t *base_type)
{
    type_info_t *type = type_copy(base_type);
    if (type == NULL) {
        return NULL;
    }
    
    type->is_const = true;
    
    return type;
}

void type_destroy(type_info_t *type)
{
    if (type == NULL) {
        return;
    }
    
    switch (type->category) {
        case TYPE_CATEGORY_CLASS:
            if (type->data.class_info.class_name != NULL) {
                free(type->data.class_info.class_name);
            }
            break;
        case TYPE_CATEGORY_ARRAY:
            if (type->data.array_info.element_type != NULL) {
                type_destroy(type->data.array_info.element_type);
            }
            break;
        default:
            break;
    }
    
    free(type);
}

type_info_t* type_copy(const type_info_t *type)
{
    if (type == NULL) {
        return NULL;
    }
    
    type_info_t *copy = malloc(sizeof(type_info_t));
    if (copy == NULL) {
        return NULL;
    }
    
    *copy = *type;
    
    // Deep copy for complex types
    switch (type->category) {
        case TYPE_CATEGORY_CLASS:
            if (type->data.class_info.class_name != NULL) {
                copy->data.class_info.class_name = malloc(type->data.class_info.class_name_len + 1);
                if (copy->data.class_info.class_name == NULL) {
                    free(copy);
                    return NULL;
                }
                strcpy(copy->data.class_info.class_name, type->data.class_info.class_name);
            }
            break;
        case TYPE_CATEGORY_ARRAY:
            if (type->data.array_info.element_type != NULL) {
                copy->data.array_info.element_type = type_copy(type->data.array_info.element_type);
                if (copy->data.array_info.element_type == NULL) {
                    free(copy);
                    return NULL;
                }
            }
            break;
        default:
            break;
    }
    
    return copy;
}

bool type_is_primitive(const type_info_t *type)
{
    return type != NULL && type->category == TYPE_CATEGORY_PRIMITIVE;
}

bool type_is_object(const type_info_t *type)
{
    return type != NULL && type->category == TYPE_CATEGORY_OBJECT;
}

bool type_is_array(const type_info_t *type)
{
    return type != NULL && type->category == TYPE_CATEGORY_ARRAY;
}

bool type_is_class(const type_info_t *type)
{
    return type != NULL && type->category == TYPE_CATEGORY_CLASS;
}

bool type_is_reference(const type_info_t *type)
{
    return type != NULL && type->is_reference;
}

bool type_is_const(const type_info_t *type)
{
    return type != NULL && type->is_const;
}

bool type_equals(const type_info_t *type1, const type_info_t *type2)
{
    if (type1 == NULL || type2 == NULL) {
        return type1 == type2;
    }
    
    if (type1->category != type2->category) {
        return false;
    }
    
    switch (type1->category) {
        case TYPE_CATEGORY_PRIMITIVE:
            return type1->data.primitive == type2->data.primitive;
        case TYPE_CATEGORY_OBJECT:
            return type1->data.object == type2->data.object;
        case TYPE_CATEGORY_CLASS:
            return strcmp(type1->data.class_info.class_name, type2->data.class_info.class_name) == 0;
        case TYPE_CATEGORY_ARRAY:
            return type_equals(type1->data.array_info.element_type, type2->data.array_info.element_type);
        default:
            return false;
    }
}

bool type_compatible(const type_info_t *from, const type_info_t *to)
{
    if (from == NULL || to == NULL) {
        return false;
    }
    
    // Exact match
    if (type_equals(from, to)) {
        return true;
    }
    
    // Numeric promotion
    if (type_is_numeric(from) && type_is_numeric(to)) {
        return true;
    }
    
    // Reference compatibility
    if (type_is_reference(from) && type_is_reference(to)) {
        return true;
    }
    
    return false;
}

bool type_assignable(const type_info_t *from, const type_info_t *to)
{
    if (from == NULL || to == NULL) {
        return false;
    }
    
    // Exact match
    if (type_equals(from, to)) {
        return true;
    }
    
    // Const to non-const is not assignable
    if (type_is_const(from) && !type_is_const(to)) {
        return false;
    }
    
    // Numeric assignment
    if (type_is_numeric(from) && type_is_numeric(to)) {
        return true;
    }
    
    // Reference assignment
    if (type_is_reference(from) && type_is_reference(to)) {
        return true;
    }
    
    return false;
}

type_info_t* type_promote(const type_info_t *type)
{
    if (type == NULL || !type_is_primitive(type)) {
        return type_copy(type);
    }
    
    // Integer promotion
    if (type->data.primitive == TYPE_CHAR) {
        return type_copy(TYPE_INTEGER_PREDEF);
    }
    
    // Boolean promotion
    if (type->data.primitive == TYPE_BOOLEAN) {
        return type_copy(TYPE_INTEGER_PREDEF);
    }
    
    return type_copy(type);
}

type_info_t* type_demote(const type_info_t *type)
{
    // For now, just return a copy
    return type_copy(type);
}

const char* type_to_string(const type_info_t *type)
{
    if (type == NULL) {
        return "NULL";
    }
    
    static char buffer[256];
    
    switch (type->category) {
        case TYPE_CATEGORY_PRIMITIVE:
            switch (type->data.primitive) {
                case TYPE_INTEGER: return "INTEGER";
                case TYPE_BOOLEAN: return "BOOLEAN";
                case TYPE_CHAR: return "CHAR";
                case TYPE_REAL: return "REAL";
                default: return "UNKNOWN_PRIMITIVE";
            }
        case TYPE_CATEGORY_OBJECT:
            switch (type->data.object) {
                case OBJ_TYPE_STRING: return "STRING";
                case OBJ_TYPE_ARRAY: return "ARRAY";
                default: return "UNKNOWN_OBJECT";
            }
        case TYPE_CATEGORY_CLASS:
            snprintf(buffer, sizeof(buffer), "CLASS(%s)", type->data.class_info.class_name);
            return buffer;
        case TYPE_CATEGORY_ARRAY:
            snprintf(buffer, sizeof(buffer), "ARRAY[%s]", type_to_string(type->data.array_info.element_type));
            return buffer;
        default:
            return "UNKNOWN";
    }
}

size_t type_get_size(const type_info_t *type)
{
    if (type == NULL) {
        return 0;
    }
    
    return type->size;
}

bool type_is_numeric(const type_info_t *type)
{
    if (type == NULL || !type_is_primitive(type)) {
        return false;
    }
    
    return type->data.primitive == TYPE_INTEGER || 
           type->data.primitive == TYPE_REAL ||
           type->data.primitive == TYPE_CHAR;
}

bool type_is_integral(const type_info_t *type)
{
    if (type == NULL || !type_is_primitive(type)) {
        return false;
    }
    
    return type->data.primitive == TYPE_INTEGER || 
           type->data.primitive == TYPE_CHAR;
}

bool type_is_floating(const type_info_t *type)
{
    if (type == NULL || !type_is_primitive(type)) {
        return false;
    }
    
    return type->data.primitive == TYPE_REAL;
}
