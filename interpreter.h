#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdio.h>
#include <stdbool.h>
#include "ast.h"

typedef struct {
    double x;
    double y;
    double z;
    double heading; // degrees (0-359)
    double speed;   // m/s multiplier
    double battery; // 0-100%
    bool flying;
    bool started;
} DroneState;

typedef struct ErrorNode {
    int line;
    char message[256];
    struct ErrorNode* next;
} ErrorNode;

typedef struct {
    ErrorNode* head;
    ErrorNode* tail;
    int count;
} ErrorList;

void init_error_list(ErrorList* list);
void add_error(ErrorList* list, int line, const char* msg);
void free_error_list(ErrorList* list);

void record_lexical_error(int line, const char* msg);
void record_syntax_error(int line, const char* msg);

void interpret_ast(ASTNode* root, FILE* output_fp);

#endif
