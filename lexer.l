%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "ast.h"
#include "parser.tab.h"

extern void record_lexical_error(int line, const char* msg);
%}

%option noyywrap
%option yylineno
%option caseless
%option nounput
%option noinput

%%

"TAKEOFF"        { return TAKEOFF; }
"LAND"           { return LAND; }
"MOVE"           { return MOVE; }
"TURN"           { return TURN; }
"REPEAT"         { return REPEAT; }
"HOVER"          { return HOVER; }
"FORWARD"        { return FORWARD; }
"BACKWARD"       { return BACKWARD; }
"UP"             { return UP; }
"DOWN"           { return DOWN; }
"LEFT"           { return LEFT; }
"RIGHT"          { return RIGHT; }
"SET"            { return SET; }
"SPEED"          { return SPEED; }
"BATTERY"        { return BATTERY; }
"RETURN_HOME"    { return RETURN_HOME; }
"START"          { return START; }
"STOP"           { return STOP; }

"["              { return LBRACKET; }
"]"              { return RBRACKET; }

[0-9]+(\.[0-9]+)? {
    yylval.num_val = atof(yytext);
    return NUMBER;
}

"//".*           { }

[ \t\r\n]+       { }

[a-zA-Z_][a-zA-Z0-9_]* {
    yylval.str_val = strdup(yytext);
    return IDENT;
}

. {
    char err_buf[128];
    snprintf(err_buf, sizeof(err_buf), "Lexical error: Unexpected character '%s'", yytext);
    record_lexical_error(yylineno, err_buf);
}

%%
