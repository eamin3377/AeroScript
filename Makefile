CC = gcc
CFLAGS = -Wall -Wextra -g -I.
LEX = flex
YACC = bison

TARGET = dronec
OBJS = lex.yy.o parser.tab.o ast.o interpreter.o main.o

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -lm

parser.tab.c parser.tab.h: parser.y ast.h
	$(YACC) -d parser.y

lex.yy.c: lexer.l parser.tab.h ast.h
	$(LEX) lexer.l

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(TARGET) *.o parser.tab.c parser.tab.h lex.yy.c

test: all
	./$(TARGET) examples/valid_flight.drone flight.json
	@echo "--- Flight Output Generated ---"
	cat flight.json

.PHONY: all clean test
