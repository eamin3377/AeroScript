%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"

extern int yylex(void);
extern int yylineno;
extern char* yytext;
void yyerror(const char *s);

ASTNode* ast_root = NULL;

typedef struct ErrorItem {
    int line;
    char message[256];
    struct ErrorItem* next;
} ErrorItem;

extern ErrorItem* syntax_error_head;
void record_syntax_error(int line, const char* msg);

%}

%union {
    double num_val;
    char* str_val;
    ASTNode* ast_node;
    DirectionType dir_val;
    TurnType turn_val;
}

%token TAKEOFF LAND MOVE TURN REPEAT HOVER FORWARD BACKWARD UP DOWN LEFT RIGHT
%token SET SPEED BATTERY RETURN_HOME START STOP LBRACKET RBRACKET
%token <num_val> NUMBER
%token <str_val> IDENT

%type <ast_node> program statement_list statement move_stmt turn_stmt hover_stmt repeat_stmt set_speed_stmt set_battery_stmt
%type <dir_val> direction
%type <turn_val> turn_dir

%start program

%%

program:
    statement_list {
        ast_root = create_program_node($1);
        $$ = ast_root;
    }
    | /* empty */ {
        ast_root = create_program_node(NULL);
        $$ = ast_root;
    }
    ;

statement_list:
    statement_list statement {
        $$ = append_statement($1, $2);
    }
    | statement {
        $$ = append_statement(NULL, $1);
    }
    ;

statement:
    START {
        $$ = create_node(AST_START, yylineno);
    }
    | STOP {
        $$ = create_node(AST_STOP, yylineno);
    }
    | TAKEOFF {
        $$ = create_node(AST_TAKEOFF, yylineno);
    }
    | LAND {
        $$ = create_node(AST_LAND, yylineno);
    }
    | RETURN_HOME {
        $$ = create_node(AST_RETURN_HOME, yylineno);
    }
    | move_stmt { $$ = $1; }
    | turn_stmt { $$ = $1; }
    | hover_stmt { $$ = $1; }
    | repeat_stmt { $$ = $1; }
    | set_speed_stmt { $$ = $1; }
    | set_battery_stmt { $$ = $1; }
    | error {
        $$ = NULL; // Error recovery line
    }
    ;

move_stmt:
    MOVE direction NUMBER {
        $$ = create_move_node($2, $3, yylineno);
    }
    ;

direction:
    FORWARD   { $$ = DIR_FORWARD; }
    | BACKWARD { $$ = DIR_BACKWARD; }
    | UP       { $$ = DIR_UP; }
    | DOWN     { $$ = DIR_DOWN; }
    | LEFT     { $$ = DIR_LEFT; }
    | RIGHT    { $$ = DIR_RIGHT; }
    ;

turn_stmt:
    TURN turn_dir NUMBER {
        $$ = create_turn_node($2, $3, yylineno);
    }
    ;

turn_dir:
    LEFT  { $$ = TURN_LEFT; }
    | RIGHT { $$ = TURN_RIGHT; }
    ;

hover_stmt:
    HOVER NUMBER {
        $$ = create_hover_node($2, yylineno);
    }
    ;

repeat_stmt:
    REPEAT NUMBER LBRACKET statement_list RBRACKET {
        $$ = create_repeat_node((int)$2, $4, yylineno);
    }
    ;

set_speed_stmt:
    SET SPEED NUMBER {
        $$ = create_set_speed_node($3, yylineno);
    }
    ;

set_battery_stmt:
    SET BATTERY NUMBER {
        $$ = create_set_battery_node($3, yylineno);
    }
    ;

%%

void yyerror(const char *s) {
    char buf[256];
    snprintf(buf, sizeof(buf), "Syntax Error: %s near '%s'", s, yytext ? yytext : "");
    record_syntax_error(yylineno, buf);
}
