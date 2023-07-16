// Leonardo Reis da Silva

#pragma once

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "symbols.h"
#include "ast_types.h"
#include "debug.h"
#define HASH_SIZE 997

#define DATATYPE_INT 1
#define DATATYPE_REAL 2
#define DATATYPE_BOOL 3
#define DATATYPE_CHAR 4

char* datatype_str[] = { "invalid", "int",  "real", "bool", "char" };

typedef struct hash_t
{
    int type; 
    int datatype;
    char *text;
    int line_number;
    struct hash_t *next;
} hash_t;



int hash(char *key);
hash_t *hash_find(char *key);
hash_t *hash_insert(char *text, int type, int line_number);
char *get_key(hash_t *hash);
void hash_print();
int hash_check_undeclared(void);
int ast_type_to_datatype(int ast_type);

hash_t *hash_table[HASH_SIZE];

void initMe(void)
{
    int i;
    for (i = 0; i < HASH_SIZE; i++)
    {
        hash_table[i] = 0;
    }
}

int hash(char *text)
{
    int i;
    int hash_val = 1;
    for (i = 0; i < strlen(text); i++)
    {
        hash_val = (hash_val * text[i]) % HASH_SIZE + 1;
    }

    return hash_val - 1;
}

char *get_key(hash_t *hash)
{
    return hash->text;
}

hash_t *hash_find(char *key)
{
    int pos = hash(key);

    hash_t *item = hash_table[pos];
    if (item == NULL)
    {
        return NULL;
    }

    if (item->next == NULL)
    {
        return item;
    }

    item = item->next;
    while (item != NULL)
    {
        char *item_key = get_key(item);
        if (strcmp(key, item_key))
        {
            return item;
        }

        item = item->next;
    }

    return NULL;
}

hash_t *hash_insert(char *text, int type, int datatype)
{
    hash_t *item = (hash_t *)calloc(1, sizeof(hash_t));
    item->type = type;
    item->text = (char *)calloc(strlen(text) + 1, sizeof(char));
    item->line_number = line_number;
    item->datatype = datatype;
    strcpy(item->text, text);

    char *key = get_key(item);
    int pos = hash(key);

    debug_printf("hash.h: INSERTING WITH KEY %s ON POS %d", key, pos);

    if (hash_table[pos])
    {
        debug_printf("hash.h: COLLISION ON POS %d", pos);
        hash_t *current_item = hash_table[pos];
        hash_t *prev_item = NULL;
        while (current_item != NULL)
        {
            if (strcmp(get_key(current_item), key) == 0 /* current_item_key != key */)
            {
                return current_item;
            }

            prev_item = current_item;
            current_item = current_item->next;
        }

        debug_printf("hash.h: DIFFERENT KEY ON POS %d", pos);
        prev_item->next = (hash_t *)calloc(1, sizeof(hash_t));
        debug_printf("hash.h: LINKED LIST %d", pos);
        prev_item->next = item;
        return prev_item->next;
    }

    hash_table[pos] = (hash_t *)calloc(1, sizeof(hash_t));
    hash_table[pos] = item;

    return hash_table[pos];
}

void hash_print()
{
    if (hash == NULL)
    {
        return;
    }

    hash_t* node;

    int i;
    for (i = 0; i < HASH_SIZE; i++) {
        for (node = hash_table[i]; node; node = node->next) {
            printf("Table[%d] has %s with type %s of %s datatype\n", i, node->text, symbol_type_str(node->type), datatype_str[node->datatype]);
        }
    }
}

int hash_check_undeclared(void)
{
   if (hash == NULL)
    {
        return 0;
    }

    int undeclared = 0;

    hash_t* node;

    int i;
    for (i = 0; i < HASH_SIZE; i++) {
        for (node = hash_table[i]; node; node = node->next) {
            if (node->type == SYMBOL_IDENTIFIER) {
                fprintf(stderr, "Semantic error: Undeclared identifier %s\n", node->text);
                undeclared++;
            }

        }
    }

    return undeclared;
}

int ast_type_to_datatype(int ast_type) {
    switch (ast_type) {
        case AST_VAR_DECL_INT: return DATATYPE_INT;
        case AST_VAR_DECL_REAL: return DATATYPE_REAL;
        case AST_VAR_DECL_BOOL: return DATATYPE_BOOL;
        case AST_VAR_DECL_CHAR: return DATATYPE_CHAR;

        case AST_VEC_DECL_INT: return DATATYPE_INT;
        case AST_VEC_DECL_REAL: return DATATYPE_REAL;
        case AST_VEC_DECL_BOOL: return DATATYPE_BOOL;
        case AST_VEC_DECL_CHAR: return DATATYPE_CHAR;

        case AST_FUNC_DECL_INT: return DATATYPE_INT;
        case AST_FUNC_DECL_REAL: return DATATYPE_REAL;
        case AST_FUNC_DECL_BOOL: return DATATYPE_BOOL;
        case AST_FUNC_DECL_CHAR: return DATATYPE_CHAR;
    }

    return 0;
}