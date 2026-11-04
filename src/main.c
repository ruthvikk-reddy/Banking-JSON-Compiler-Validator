#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include "../include/common.h"
#include "json_value.h"
#include "semantic_analyzer.h"
#include "pattern_matcher.h"
#include "domain_rules.h"
#include "cross_field_validator.h"
#include "error_reporter.h"
#include "validation_report.h"

/* Externals from parser and lexer */
extern int yyparse(void);
extern FILE *yyin;
extern int yylineno;
extern int yycolumn;
extern JsonValue *g_parsed_root;
extern int g_lexical_error_count;
extern int g_syntax_error_count;
extern int g_token_count;

extern int g_tok_lbrace;
extern int g_tok_rbrace;
extern int g_tok_lbracket;
extern int g_tok_rbracket;
extern int g_tok_colon;
extern int g_tok_comma;
extern int g_tok_string;
extern int g_tok_number;
extern int g_tok_bool;
extern int g_tok_null;

extern char g_last_syntax_error[256];
extern int g_syntax_error_line;
extern int g_syntax_error_col;
extern char g_last_lexical_error[256];
extern int g_lexical_error_line;
extern int g_lexical_error_col;

/* Global configuration flags */
bool g_report_json_mode = false;

static void print_usage(const char *prog_name) {
    printf("========================================================================\n");
    printf("%s\n", PROJECT_TITLE);
    printf("Compiler Design Project — Complete 8-Stage Architecture\n");
    printf("Team %d: %s, %s, %s\n", TEAM_NUMBER, AUTHOR_1, AUTHOR_2, AUTHOR_3);
    printf("========================================================================\n\n");
    printf("Usage:\n");
    printf("  %s [options] <input_json_file>\n", prog_name);
    printf("  %s --interactive\n\n", prog_name);
    printf("Options:\n");
    printf("  -h, --help           Display this help information\n");
    printf("  -i, --interactive    Interactive mode (read JSON from standard input)\n");
    printf("  -v, --version        Display project version and team details\n");
    printf("  --report-json        Emit complete validation audit report in JSON format\n\n");
    printf("Examples:\n");
    printf("  %s examples/valid_customer.json\n", prog_name);
    printf("  %s --report-json examples/valid_customer.json\n", prog_name);
    printf("  %s examples/invalid_syntax.json\n", prog_name);
    printf("  %s -i < examples/valid_customer.json\n\n", prog_name);
}

static void print_version(void) {
    printf("%s\n", PROJECT_TITLE);
    printf("Domain       : %s\n", PROJECT_DOMAIN);
    printf("Team Number  : %d\n", TEAM_NUMBER);
    printf("Team Members :\n");
    printf("  - %s\n", AUTHOR_1);
    printf("  - %s\n", AUTHOR_2);
    printf("  - %s\n", AUTHOR_3);
    printf("Architecture : Complete 8-Stage Pipeline\n");
    printf("               Stage 1: JSON Banking Input\n");
    printf("               Stage 2: Lexical Analyzer (Flex)\n");
    printf("               Stage 3: Syntax Analyzer & AST (Bison)\n");
    printf("               Stage 4: Semantic Analyzer (10-Field Schema & Types)\n");
    printf("               Stage 5A: Pattern Matching Engine (Regex)\n");
    printf("               Stage 5B: Domain Rule Engine\n");
    printf("               Stage 6: Cross-Field Validation Engine\n");
    printf("               Stage 7: Error Detection & Aggregation Engine\n");
    printf("               Stage 8: Final Validation Report Output\n");
}

int main(int argc, char *argv[]) {
    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    const char *file_path = NULL;
    bool interactive = false;

    if (argc < 2) {
        print_usage(argv[0]);
        return 1;
    }

    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-h") == 0 || strcmp(argv[i], "--help") == 0) {
            print_usage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "-v") == 0 || strcmp(argv[i], "--version") == 0) {
            print_version();
            return 0;
        } else if (strcmp(argv[i], "-i") == 0 || strcmp(argv[i], "--interactive") == 0) {
            interactive = true;
        } else if (strcmp(argv[i], "--report-json") == 0) {
            g_report_json_mode = true;
        } else if (strcmp(argv[i], "-") == 0) {
            file_path = "-";
        } else if (argv[i][0] == '-') {
            fprintf(stderr, "Error: Unknown option '%s'. Use --help for usage.\n", argv[i]);
            return 1;
        } else {
            file_path = argv[i];
        }
    }

    /* Initialize central diagnostic aggregator (Stage 7 setup) */
    error_reporter_init(file_path);

    FILE *input_fp = NULL;

    /* Stage 1: JSON Banking Input */
    if (interactive || (file_path && strcmp(file_path, "-") == 0)) {
        input_fp = stdin;
        error_reporter_set_stage_status(STAGE_INPUT, STAGE_STATUS_PASS);
        if (!g_report_json_mode) {
            printf("JSON Banking Input: PASS\n\n");
        }
    } else {
        if (!file_path) {
            fprintf(stderr, "Error: No input JSON file provided. Use --help for usage.\n");
            error_reporter_cleanup();
            return 1;
        }
        input_fp = fopen(file_path, "r");
        if (!input_fp) {
            char io_err[256];
            snprintf(io_err, sizeof(io_err), "Cannot open file '%s': %s", file_path, strerror(errno));
            if (!g_report_json_mode) {
                printf("JSON Banking Input: FAIL (%s)\n\n", io_err);
            }
            error_reporter_add_error(STAGE_INPUT, "RULE_FILE_ACCESS", NULL, 0, 0, io_err);
            error_reporter_set_stage_status(STAGE_INPUT, STAGE_STATUS_FAIL);

            /* Report final diagnostic summary and output */
            if (!g_report_json_mode) {
                printf("\n");
            }
            run_stage7_error_aggregation();
            run_stage8_validation_report(g_report_json_mode, stdout);
            error_reporter_cleanup();
            return 1;
        }

        /* Check for empty file */
        fseek(input_fp, 0, SEEK_END);
        long file_len = ftell(input_fp);
        rewind(input_fp);

        if (file_len == 0) {
            if (!g_report_json_mode) {
                printf("JSON Banking Input: FAIL\n");
                printf("  Empty input: JSON grammar requires a root value (0 bytes).\n");
                printf("  SYNTAX ERROR (Line 1, Col 1): Empty input\n\n");
            }
            error_reporter_add_error(STAGE_INPUT, "RULE_EMPTY_INPUT", NULL, 1, 1,
                                     "Empty input: JSON grammar requires a root value (0 bytes).");
            error_reporter_set_stage_status(STAGE_INPUT, STAGE_STATUS_FAIL);

            if (!g_report_json_mode) {
                printf("\n");
            }
            run_stage7_error_aggregation();
            run_stage8_validation_report(g_report_json_mode, stdout);
            fclose(input_fp);
            error_reporter_cleanup();
            return 1;
        }

        error_reporter_set_stage_status(STAGE_INPUT, STAGE_STATUS_PASS);
        if (!g_report_json_mode) {
            printf("JSON Banking Input: PASS\n\n");
        }
    }

    /* Reset parser and lexer states */
    yylineno = 1;
    yycolumn = 1;
    g_parsed_root = NULL;
    g_lexical_error_count = 0;
    g_syntax_error_count = 0;
    g_token_count = 0;

    g_tok_lbrace = 0;
    g_tok_rbrace = 0;
    g_tok_lbracket = 0;
    g_tok_rbracket = 0;
    g_tok_colon = 0;
    g_tok_comma = 0;
    g_tok_string = 0;
    g_tok_number = 0;
    g_tok_bool = 0;
    g_tok_null = 0;

    g_last_syntax_error[0] = '\0';
    g_last_lexical_error[0] = '\0';
    yyin = input_fp;

    /* Execute Lexical and Syntax Analysis via yyparse() */
    int parse_status = yyparse();

    /* Sync token statistics into diagnostic aggregator */
    error_reporter_sync_tokens(g_tok_lbrace, g_tok_rbrace, g_tok_lbracket, g_tok_rbracket,
                               g_tok_colon, g_tok_comma, g_tok_string, g_tok_number,
                               g_tok_bool, g_tok_null, g_token_count);

    /* Stage 2: Lexical Analysis Evaluation */
    if (g_lexical_error_count > 0) {
        if (!g_report_json_mode) {
            printf("Lexical Analyzer: FAIL\n");
            printf("  LEXICAL ERROR (Line %d, Col %d): %s\n\n",
                    g_lexical_error_line, g_lexical_error_col, g_last_lexical_error);
        }
        error_reporter_add_error(STAGE_LEXER, "RULE_LEX_TOKEN", NULL,
                                 g_lexical_error_line, g_lexical_error_col, g_last_lexical_error);
        error_reporter_set_stage_status(STAGE_LEXER, STAGE_STATUS_FAIL);

        if (!g_report_json_mode) {
            printf("\n");
        }
        run_stage7_error_aggregation();
        run_stage8_validation_report(g_report_json_mode, stdout);
        json_value_free_all();
        if (input_fp && input_fp != stdin) {
            fclose(input_fp);
        }
        error_reporter_cleanup();
        return 1;
    }

    error_reporter_set_stage_status(STAGE_LEXER, STAGE_STATUS_PASS);
    if (!g_report_json_mode) {
        printf("Lexical Analyzer: PASS\n");
        printf("Token Breakdown:\n");
        printf("  Opening braces: %d\n", g_tok_lbrace);
        printf("  Closing braces: %d\n", g_tok_rbrace);
        printf("  Opening brackets: %d\n", g_tok_lbracket);
        printf("  Closing brackets: %d\n", g_tok_rbracket);
        printf("  Colons: %d\n", g_tok_colon);
        printf("  Commas: %d\n", g_tok_comma);
        printf("  String values: %d\n", g_tok_string);
        printf("  Number values: %d\n", g_tok_number);
        printf("  Boolean values: %d\n", g_tok_bool);
        printf("  Null values: %d\n", g_tok_null);
        printf("Total tokens recognized: %d\n\n", g_token_count);
    }

    /* Stage 3: Syntax Analysis Evaluation */
    if (parse_status != 0 || g_syntax_error_count > 0 || !g_parsed_root) {
        if (!g_report_json_mode) {
            printf("Syntax Analyzer: FAIL\n");
            printf("  JSON grammar: invalid\n");
            if (g_last_syntax_error[0] != '\0') {
                printf("  SYNTAX ERROR (Line %d, Col %d): %s\n\n",
                       g_syntax_error_line, g_syntax_error_col, g_last_syntax_error);
            } else {
                printf("  SYNTAX ERROR: Parsing failed.\n\n");
            }
        }
        const char *syn_msg = (g_last_syntax_error[0] != '\0') ? g_last_syntax_error : "Parsing failed.";
        error_reporter_add_error(STAGE_PARSER, "RULE_SYNTAX_PARSER", NULL,
                                 g_syntax_error_line, g_syntax_error_col, syn_msg);
        error_reporter_set_stage_status(STAGE_PARSER, STAGE_STATUS_FAIL);

        if (!g_report_json_mode) {
            printf("\n");
        }
        run_stage7_error_aggregation();
        run_stage8_validation_report(g_report_json_mode, stdout);
        json_value_free_all();
        if (input_fp && input_fp != stdin) {
            fclose(input_fp);
        }
        error_reporter_cleanup();
        return 1;
    }

    error_reporter_set_stage_status(STAGE_PARSER, STAGE_STATUS_PASS);
    if (!g_report_json_mode) {
        printf("Syntax Analyzer: PASS\n");
        printf("  JSON grammar: valid\n\n");
    }

    /* Stage 4: Semantic Analysis (Structure, 10 Core Fields, Data Types, Blank Checks) */
    bool s4 = run_stage4_semantic_analysis(g_parsed_root);
    if (!s4) {
        /* AST prerequisites invalid: do not invoke downstream field checks */
        if (!g_report_json_mode) {
            printf("\n");
        }
        run_stage7_error_aggregation();
        run_stage8_validation_report(g_report_json_mode, stdout);
        json_value_free_all();
        if (input_fp && input_fp != stdin) {
            fclose(input_fp);
        }
        error_reporter_cleanup();
        return 1;
    }

    /* Stage 5A: Pattern Matching Engine (Regex on 8 string fields) */
    if (!g_report_json_mode) {
        printf("\n");
    }
    bool s5a = run_stage5a_pattern_matching(g_parsed_root);

    /* Stage 5B: Domain Rule Engine (account type, non-negative, minimum deposit) */
    if (!g_report_json_mode) {
        printf("\n");
    }
    bool s5b = run_stage5b_domain_rules(g_parsed_root);

    /* Stage 6: Cross-Field Validation Engine */
    if (!g_report_json_mode) {
        printf("\n");
    }
    bool s6 = run_stage6_cross_field_validation(g_parsed_root);

    /* Stage 7: Error Detection and Aggregation */
    if (!g_report_json_mode) {
        printf("\n");
    }
    run_stage7_error_aggregation();

    /* Stage 8: Validation Report / Final Output */
    run_stage8_validation_report(g_report_json_mode, stdout);

    bool overall_valid = s4 && s5a && s5b && s6 && (g_diagnostic_catalog.error_count == 0);

    /* Cleanup AST memory and error reporter resources */
    json_value_free_all();
    g_parsed_root = NULL;

    if (input_fp && input_fp != stdin) {
        fclose(input_fp);
    }
    error_reporter_cleanup();

    return overall_valid ? 0 : 1;
}
