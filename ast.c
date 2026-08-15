#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

ASTNode* create_node(ASTNodeType type, int line) {
    ASTNode* node = (ASTNode*)calloc(1, sizeof(ASTNode));
    node->type = type;
    node->line = line;
    return node;
}

ASTNode* create_move_node(DirectionType dir, double amount, int line) {
    ASTNode* node = create_node(AST_MOVE, line);
    node->data.move.dir = dir;
    node->data.move.amount = amount;
    return node;
}

ASTNode* create_turn_node(TurnType turn_dir, double angle, int line) {
    ASTNode* node = create_node(AST_TURN, line);
    node->data.turn.turn_dir = turn_dir;
    node->data.turn.angle = angle;
    return node;
}

ASTNode* create_hover_node(double duration, int line) {
    ASTNode* node = create_node(AST_HOVER, line);
    node->data.hover.duration = duration;
    return node;
}

ASTNode* create_repeat_node(int count, ASTNode* stmt_list, int line) {
    ASTNode* node = create_node(AST_REPEAT, line);
    node->data.repeat.count = count;
    if (stmt_list && stmt_list->type == AST_PROGRAM) {
        node->data.repeat.statement_count = stmt_list->data.program.statement_count;
        node->data.repeat.statements = stmt_list->data.program.statements;
        free(stmt_list); // Free the outer container shell
    }
    return node;
}

ASTNode* create_set_speed_node(double speed, int line) {
    ASTNode* node = create_node(AST_SET_SPEED, line);
    node->data.set_speed.speed = speed;
    return node;
}

ASTNode* create_set_battery_node(double battery, int line) {
    ASTNode* node = create_node(AST_SET_BATTERY, line);
    node->data.set_battery.battery = battery;
    return node;
}

ASTNode* create_program_node(ASTNode* stmt_list) {
    if (stmt_list && stmt_list->type == AST_PROGRAM) {
        return stmt_list;
    }
    ASTNode* node = create_node(AST_PROGRAM, 1);
    node->data.program.statements = NULL;
    node->data.program.statement_count = 0;
    return node;
}

ASTNode* append_statement(ASTNode* list_node, ASTNode* stmt) {
    if (!stmt) return list_node;
    if (!list_node) {
        list_node = create_node(AST_PROGRAM, stmt->line);
    }
    list_node->data.program.statement_count++;
    list_node->data.program.statements = (ASTNode**)realloc(
        list_node->data.program.statements,
        sizeof(ASTNode*) * list_node->data.program.statement_count
    );
    list_node->data.program.statements[list_node->data.program.statement_count - 1] = stmt;
    return list_node;
}

void print_ast(ASTNode* node, int indent) {
    if (!node) return;
    for (int i = 0; i < indent; i++) printf("  ");
    switch (node->type) {
        case AST_PROGRAM:
            printf("Program (%d statements)\n", node->data.program.statement_count);
            for (int i = 0; i < node->data.program.statement_count; i++) {
                print_ast(node->data.program.statements[i], indent + 1);
            }
            break;
        case AST_TAKEOFF:
            printf("TAKEOFF [line %d]\n", node->line);
            break;
        case AST_LAND:
            printf("LAND [line %d]\n", node->line);
            break;
        case AST_RETURN_HOME:
            printf("RETURN_HOME [line %d]\n", node->line);
            break;
        case AST_START:
            printf("START [line %d]\n", node->line);
            break;
        case AST_STOP:
            printf("STOP [line %d]\n", node->line);
            break;
        case AST_MOVE:
            printf("MOVE ");
            switch(node->data.move.dir) {
                case DIR_FORWARD: printf("FORWARD "); break;
                case DIR_BACKWARD: printf("BACKWARD "); break;
                case DIR_UP: printf("UP "); break;
                case DIR_DOWN: printf("DOWN "); break;
                case DIR_LEFT: printf("LEFT "); break;
                case DIR_RIGHT: printf("RIGHT "); break;
            }
            printf("%.2f [line %d]\n", node->data.move.amount, node->line);
            break;
        case AST_TURN:
            printf("TURN %s %.2f [line %d]\n",
                node->data.turn.turn_dir == TURN_LEFT ? "LEFT" : "RIGHT",
                node->data.turn.angle, node->line);
            break;
        case AST_HOVER:
            printf("HOVER %.2fs [line %d]\n", node->data.hover.duration, node->line);
            break;
        case AST_SET_SPEED:
            printf("SET SPEED %.2f [line %d]\n", node->data.set_speed.speed, node->line);
            break;
        case AST_SET_BATTERY:
            printf("SET BATTERY %.2f%% [line %d]\n", node->data.set_battery.battery, node->line);
            break;
        case AST_REPEAT:
            printf("REPEAT %d times [line %d]\n", node->data.repeat.count, node->line);
            for (int i = 0; i < node->data.repeat.statement_count; i++) {
                print_ast(node->data.repeat.statements[i], indent + 1);
            }
            break;
    }
}

void free_ast(ASTNode* node) {
    if (!node) return;
    if (node->type == AST_PROGRAM) {
        for (int i = 0; i < node->data.program.statement_count; i++) {
            free_ast(node->data.program.statements[i]);
        }
        free(node->data.program.statements);
    } else if (node->type == AST_REPEAT) {
        for (int i = 0; i < node->data.repeat.statement_count; i++) {
            free_ast(node->data.repeat.statements[i]);
        }
        free(node->data.repeat.statements);
    }
    free(node);
}
