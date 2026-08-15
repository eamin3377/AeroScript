#ifndef AST_H
#define AST_H

#include <stdio.h>
#include <stdlib.h>

typedef enum {
    DIR_FORWARD,
    DIR_BACKWARD,
    DIR_UP,
    DIR_DOWN,
    DIR_LEFT,
    DIR_RIGHT
} DirectionType;

typedef enum {
    TURN_LEFT,
    TURN_RIGHT
} TurnType;

typedef enum {
    AST_PROGRAM,
    AST_TAKEOFF,
    AST_LAND,
    AST_MOVE,
    AST_TURN,
    AST_HOVER,
    AST_REPEAT,
    AST_SET_SPEED,
    AST_SET_BATTERY,
    AST_RETURN_HOME,
    AST_START,
    AST_STOP
} ASTNodeType;

typedef struct ASTNode {
    ASTNodeType type;
    int line;
    union {
        struct {
            DirectionType dir;
            double amount;
        } move;
        struct {
            TurnType turn_dir;
            double angle;
        } turn;
        struct {
            double duration;
        } hover;
        struct {
            int count;
            struct ASTNode** statements;
            int statement_count;
        } repeat;
        struct {
            double speed;
        } set_speed;
        struct {
            double battery;
        } set_battery;
        struct {
            struct ASTNode** statements;
            int statement_count;
        } program;
    } data;
} ASTNode;

ASTNode* create_node(ASTNodeType type, int line);
ASTNode* create_move_node(DirectionType dir, double amount, int line);
ASTNode* create_turn_node(TurnType turn_dir, double angle, int line);
ASTNode* create_hover_node(double duration, int line);
ASTNode* create_repeat_node(int count, ASTNode* stmt_list, int line);
ASTNode* create_set_speed_node(double speed, int line);
ASTNode* create_set_battery_node(double battery, int line);
ASTNode* create_program_node(ASTNode* stmt_list);

ASTNode* append_statement(ASTNode* list_node, ASTNode* stmt);
void free_ast(ASTNode* node);
void print_ast(ASTNode* node, int indent);

#endif
