#include <stdio.h>
#include <stdlib.h>
#include "ast.h"
#include "interpreter.h"

extern int yyparse(void);
extern FILE* yyin;
extern ASTNode* ast_root;

int main(int argc, char** argv) {
    if (argc > 1) {
        FILE* fp = fopen(argv[1], "r");
        if (!fp) {
            fprintf(stderr, "Error opening input file '%s'\n", argv[1]);
            return 1;
        }
        yyin = fp;
    } else {
        yyin = stdin;
    }

    FILE* out_fp = stdout;
    if (argc > 2) {
        out_fp = fopen(argv[2], "w");
        if (!out_fp) {
            fprintf(stderr, "Error opening output file '%s'\n", argv[2]);
            return 1;
        }
    }

    yyparse();
    if (yyin && yyin != stdin) {
        fclose(yyin);
    }

    interpret_ast(ast_root, out_fp);

    if (ast_root) {
        free_ast(ast_root);
    }

    if (out_fp && out_fp != stdout) {
        fclose(out_fp);
    }

    return 0;
}
