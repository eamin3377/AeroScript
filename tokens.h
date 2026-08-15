#ifndef TOKENS_H
#ifndef TOKENS_H
#define TOKENS_H

enum TokenType {
    TOK_EOF = 0,
    TOK_TAKEOFF,
    TOK_LAND,
    TOK_MOVE,
    TOK_TURN,
    TOK_REPEAT,
    TOK_HOVER,
    TOK_FORWARD,
    TOK_BACKWARD,
    TOK_UP,
    TOK_DOWN,
    TOK_LEFT,
    TOK_RIGHT,
    TOK_SET,
    TOK_SPEED,
    TOK_BATTERY,
    TOK_RETURN_HOME,
    TOK_START,
    TOK_STOP,
    TOK_LBRACKET,
    TOK_RBRACKET,
    TOK_NUMBER,
    TOK_IDENT,
    TOK_ERROR
};

#endif
