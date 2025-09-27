/*
 * ARX AST (Abstract Syntax Tree) Implementation
 * Handles AST node creation, manipulation, and destruction
 */

#include "ast.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

ast_node_t* ast_create_node(ast_node_type_t type)
{
    ast_node_t *node = malloc(sizeof(ast_node_t));
    if (node == NULL) {
        return NULL;
    }
    
    node->type = type;
    node->value = NULL;
    node->number = 0;
    node->children = NULL;
    node->child_count = 0;
    node->child_capacity = 0;
    node->line_number = 0;
    node->column_number = 0;
    
    return node;
}

void ast_add_child(ast_node_t *parent, ast_node_t *child)
{
    if (parent == NULL || child == NULL) {
        return;
    }
    
    // Resize children array if needed
    if (parent->child_count >= parent->child_capacity) {
        size_t new_capacity = parent->child_capacity == 0 ? 4 : parent->child_capacity * 2;
        ast_node_t **new_children = realloc(parent->children, new_capacity * sizeof(ast_node_t*));
        if (new_children == NULL) {
            return; // Memory allocation failed
        }
        parent->children = new_children;
        parent->child_capacity = new_capacity;
    }
    
    parent->children[parent->child_count] = child;
    parent->child_count++;
}

void ast_set_value(ast_node_t *node, const char *value)
{
    if (node == NULL) {
        return;
    }
    
    if (node->value != NULL) {
        free(node->value);
    }
    
    if (value != NULL) {
        node->value = malloc(strlen(value) + 1);
        if (node->value != NULL) {
            strcpy(node->value, value);
        }
    } else {
        node->value = NULL;
    }
}

void ast_set_number(ast_node_t *node, uint64_t number)
{
    if (node != NULL) {
        node->number = number;
    }
}

void ast_destroy_node(ast_node_t *node)
{
    if (node == NULL) {
        return;
    }
    
    // Destroy children
    for (size_t i = 0; i < node->child_count; i++) {
        ast_destroy_node(node->children[i]);
    }
    
    // Free children array
    if (node->children != NULL) {
        free(node->children);
    }
    
    // Free value
    if (node->value != NULL) {
        free(node->value);
    }
    
    // Free node
    free(node);
}
