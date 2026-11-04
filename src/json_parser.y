%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "../include/common.h"
#include "json_value.h"

int yylex(void);
extern int yylineno;
extern int yycolumn;
extern FILE *yyin;

void yyerror(const char *msg);

JsonValue *g_parsed_root = NULL;
int g_syntax_error_count = 0;
char g_last_syntax_error[256] = "";
int g_syntax_error_line = 0;
int g_syntax_error_col = 0;
%}

%locations

%code requires {
#include "json_value.h"

typedef struct ParserMember {
    char *key;
    JsonValue *val;
    int line;
    int col;
} ParserMember;
}

%union {
    char *str;
    JsonValue *val;
    ParserMember parser_member;
}

%token <str> TOKEN_STRING
%token <val> TOKEN_NUMBER
%token <val> TOKEN_TRUE
%token <val> TOKEN_FALSE
%token <val> TOKEN_NULL
%token LBRACE "{"
%token RBRACE "}"
%token LBRACKET "["
%token RBRACKET "]"
%token COLON ":"
%token COMMA ","

%type <val> json_root value object array
%type <val> member_list element_list
%type <parser_member> member

%destructor { free($$); } <str>
%destructor { if ($$.key) free($$.key); } <parser_member>

%define parse.error verbose

%%

json_root:
    value {
        g_parsed_root = $1;
        $$ = $1;
    }
    ;

value:
    object {
        $$ = $1;
    }
    | array {
        $$ = $1;
    }
    | TOKEN_STRING {
        $$ = json_value_create_string($1, @1.first_line, @1.first_column);
        free($1);
    }
    | TOKEN_NUMBER {
        $$ = $1;
    }
    | TOKEN_TRUE {
        $$ = $1;
    }
    | TOKEN_FALSE {
        $$ = $1;
    }
    | TOKEN_NULL {
        $$ = $1;
    }
    ;

object:
    LBRACE RBRACE {
        $$ = json_value_create_object(@1.first_line, @1.first_column);
    }
    | LBRACE member_list RBRACE {
        $$ = $2;
        $$->line = @1.first_line;
        $$->col = @1.first_column;
    }
    ;

member_list:
    member {
        $$ = json_value_create_object(@1.first_line, @1.first_column);
        json_object_add_member($$, $1.key, $1.val, $1.line, $1.col);
    }
    | member_list COMMA member {
        $$ = $1;
        json_object_add_member($$, $3.key, $3.val, $3.line, $3.col);
    }
    ;

member:
    TOKEN_STRING COLON value {
        $$.key = $1;
        $$.val = $3;
        $$.line = @1.first_line;
        $$.col = @1.first_column;
    }
    ;

array:
    LBRACKET RBRACKET {
        $$ = json_value_create_array(@1.first_line, @1.first_column);
    }
    | LBRACKET element_list RBRACKET {
        $$ = $2;
        $$->line = @1.first_line;
        $$->col = @1.first_column;
    }
    ;

element_list:
    value {
        $$ = json_value_create_array(@1.first_line, @1.first_column);
        json_array_add_element($$, $1);
    }
    | element_list COMMA value {
        $$ = $1;
        json_array_add_element($$, $3);
    }
    ;

%%

void yyerror(const char *msg) {
    if (g_syntax_error_count == 0) {
        snprintf(g_last_syntax_error, sizeof(g_last_syntax_error), "%s", msg);
        g_syntax_error_line = yylloc.first_line;
        g_syntax_error_col = yylloc.first_column;
    }
    g_syntax_error_count++;
}
