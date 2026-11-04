%top{
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
}
%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include "../include/common.h"
#include "json_value.h"
#include "json_parser.tab.h"

int yycolumn = 1;
int g_lexical_error_count = 0;
int g_token_count = 0;

int g_tok_lbrace = 0;
int g_tok_rbrace = 0;
int g_tok_lbracket = 0;
int g_tok_rbracket = 0;
int g_tok_colon = 0;
int g_tok_comma = 0;
int g_tok_string = 0;
int g_tok_number = 0;
int g_tok_bool = 0;
int g_tok_null = 0;

char g_last_lexical_error[256] = "";
int g_lexical_error_line = 0;
int g_lexical_error_col = 0;

static void report_lexical_error(const char *msg, int line, int col) {
    if (g_lexical_error_count == 0) {
        snprintf(g_last_lexical_error, sizeof(g_last_lexical_error), "%s", msg);
        g_lexical_error_line = line;
        g_lexical_error_col = col;
    }
    g_lexical_error_count++;
}

#define YY_USER_ACTION \
    yylloc.first_line = yylineno; \
    yylloc.first_column = yycolumn; \
    yylloc.last_line = yylineno; \
    yylloc.last_column = yycolumn + yyleng - 1; \
    yycolumn += yyleng;

static char *unescape_json_string(const char *src, int len, int line, int col) {
    char *dest = (char *)malloc(len + 1);
    if (!dest) {
        fprintf(stderr, "Out of memory in unescape_json_string\n");
        exit(EXIT_FAILURE);
    }
    int d = 0;
    for (int i = 1; i < len - 1; i++) {
        unsigned char c = (unsigned char)src[i];
        if (c < 0x20) {
            report_lexical_error("Unescaped control character in string literal", line, col + i);
        }
        if (c == '\\' && i + 1 < len - 1) {
            i++;
            char esc = src[i];
            switch (esc) {
                case '"':  dest[d++] = '"'; break;
                case '\\': dest[d++] = '\\'; break;
                case '/':  dest[d++] = '/'; break;
                case 'b':  dest[d++] = '\b'; break;
                case 'f':  dest[d++] = '\f'; break;
                case 'n':  dest[d++] = '\n'; break;
                case 'r':  dest[d++] = '\r'; break;
                case 't':  dest[d++] = '\t'; break;
                case 'u': {
                    if (i + 4 < len - 1) {
                        char hex[5] = { src[i+1], src[i+2], src[i+3], src[i+4], '\0' };
                        char *endp = NULL;
                        long cp = strtol(hex, &endp, 16);
                        if (endp && *endp == '\0') {
                            if (cp <= 0x7F) {
                                dest[d++] = (char)cp;
                            } else if (cp <= 0x7FF) {
                                dest[d++] = (char)(0xC0 | (cp >> 6));
                                dest[d++] = (char)(0x80 | (cp & 0x3F));
                            } else {
                                dest[d++] = (char)(0xE0 | (cp >> 12));
                                dest[d++] = (char)(0x80 | ((cp >> 6) & 0x3F));
                                dest[d++] = (char)(0x80 | (cp & 0x3F));
                            }
                            i += 4;
                        } else {
                            dest[d++] = '?';
                        }
                    } else {
                        dest[d++] = '?';
                    }
                    break;
                }
                default: {
                    char err_msg[64];
                    snprintf(err_msg, sizeof(err_msg), "Invalid escape sequence '\\%c' in string", esc);
                    report_lexical_error(err_msg, line, col + i - 1);
                    dest[d++] = esc;
                    break;
                }
            }
        } else {
            dest[d++] = (char)c;
        }
    }
    dest[d] = '\0';
    return dest;
}
%}

%option noyywrap
%option yylineno
%option nounput
%option noinput
%option never-interactive

%%

[ \t\r]+ {
    /* whitespace ignored, column tracked by YY_USER_ACTION */
}

\n {
    yycolumn = 1;
}

"{" {
    g_tok_lbrace++;
    g_token_count++;
    return LBRACE;
}

"}" {
    g_tok_rbrace++;
    g_token_count++;
    return RBRACE;
}

"[" {
    g_tok_lbracket++;
    g_token_count++;
    return LBRACKET;
}

"]" {
    g_tok_rbracket++;
    g_token_count++;
    return RBRACKET;
}

":" {
    g_tok_colon++;
    g_token_count++;
    return COLON;
}

"," {
    g_tok_comma++;
    g_token_count++;
    return COMMA;
}

"true" {
    g_tok_bool++;
    g_token_count++;
    yylval.val = json_value_create_bool(true, yylloc.first_line, yylloc.first_column);
    return TOKEN_TRUE;
}

"false" {
    g_tok_bool++;
    g_token_count++;
    yylval.val = json_value_create_bool(false, yylloc.first_line, yylloc.first_column);
    return TOKEN_FALSE;
}

"null" {
    g_tok_null++;
    g_token_count++;
    yylval.val = json_value_create_null(yylloc.first_line, yylloc.first_column);
    return TOKEN_NULL;
}

\"([^\"\\\n]|\\.)*\" {
    g_tok_string++;
    g_token_count++;
    yylval.str = unescape_json_string(yytext, yyleng, yylloc.first_line, yylloc.first_column);
    return TOKEN_STRING;
}

\"([^\"\\\n]|\\.)* {
    report_lexical_error("Unterminated string literal", yylloc.first_line, yylloc.first_column);
}

-?(0|[1-9][0-9]*)(\.[0-9]+)?([eE][+-]?[0-9]+)? {
    g_tok_number++;
    g_token_count++;
    double num = strtod(yytext, NULL);
    yylval.val = json_value_create_number(num, yytext, yylloc.first_line, yylloc.first_column);
    return TOKEN_NUMBER;
}

-?0[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)? {
    char err_msg[96];
    snprintf(err_msg, sizeof(err_msg), "Invalid number '%s': leading zeros are not allowed in JSON", yytext);
    report_lexical_error(err_msg, yylloc.first_line, yylloc.first_column);
}

-?(0|[1-9][0-9]*)\. {
    char err_msg[96];
    snprintf(err_msg, sizeof(err_msg), "Invalid number '%s': decimal point must be followed by digits", yytext);
    report_lexical_error(err_msg, yylloc.first_line, yylloc.first_column);
}

-?(0|[1-9][0-9]*)(\.[0-9]+)?[eE][+-]? {
    char err_msg[96];
    snprintf(err_msg, sizeof(err_msg), "Invalid number '%s': exponent must be followed by digits", yytext);
    report_lexical_error(err_msg, yylloc.first_line, yylloc.first_column);
}

\+[0-9]+(\.[0-9]+)?([eE][+-]?[0-9]+)? {
    char err_msg[96];
    snprintf(err_msg, sizeof(err_msg), "Invalid number '%s': leading '+' sign is not permitted in JSON numbers", yytext);
    report_lexical_error(err_msg, yylloc.first_line, yylloc.first_column);
}

"//"[^\n]* {
    report_lexical_error("Comments are not permitted in standard JSON ('//')", yylloc.first_line, yylloc.first_column);
}

"/*"([^*]|\*+[^*/])*\*+"/" {
    report_lexical_error("Comments are not permitted in standard JSON ('/* ... */')", yylloc.first_line, yylloc.first_column);
}

"/*" {
    report_lexical_error("Unterminated multi-line comment", yylloc.first_line, yylloc.first_column);
}

. {
    char err_msg[64];
    snprintf(err_msg, sizeof(err_msg), "Unexpected character '%c' (ASCII %d)", yytext[0], (unsigned char)yytext[0]);
    report_lexical_error(err_msg, yylloc.first_line, yylloc.first_column);
}

%%
