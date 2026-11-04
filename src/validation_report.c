#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "validation_report.h"

static const char *stage_display_status(StageStatus st) {
    switch (st) {
        case STAGE_STATUS_PASS: return "PASS";
        case STAGE_STATUS_FAIL: return "FAIL";
        default:                return "NOT EXECUTED";
    }
}

static void print_json_escaped_string(FILE *out, const char *str) {
    if (!str) {
        fputs("null", out);
        return;
    }
    fputc('"', out);
    for (const char *p = str; *p; p++) {
        switch (*p) {
            case '"':  fputs("\\\"", out); break;
            case '\\': fputs("\\\\", out); break;
            case '\b': fputs("\\b", out); break;
            case '\f': fputs("\\f", out); break;
            case '\n': fputs("\\n", out); break;
            case '\r': fputs("\\r", out); break;
            case '\t': fputs("\\t", out); break;
            default:
                if ((unsigned char)*p < 0x20) {
                    fprintf(out, "\\u%04x", (unsigned char)*p);
                } else {
                    fputc(*p, out);
                }
                break;
        }
    }
    fputc('"', out);
}

static void print_text_report(FILE *out) {
    int stages_passed = 0;
    int stages_failed = 0;

    for (int s = 1; s <= 8; s++) {
        if (g_diagnostic_catalog.stage_statuses[s] == STAGE_STATUS_PASS) {
            stages_passed++;
        } else if (g_diagnostic_catalog.stage_statuses[s] == STAGE_STATUS_FAIL) {
            stages_failed++;
        }
    }

    /* Stage 8 itself reports that it generated the final output */
    bool overall_valid = (g_diagnostic_catalog.error_count == 0 && stages_failed == 0);
    error_reporter_set_stage_status(STAGE_FINAL_REPORT, STAGE_STATUS_PASS);
    stages_passed++;

    fprintf(out, "\n========================================================================\n");
    fprintf(out, "       DOMAIN-AWARE JSON VALIDATION SYSTEM FOR BANKING DATA\n");
    fprintf(out, "========================================================================\n\n");
    fprintf(out, "Input File               : %s\n", g_diagnostic_catalog.input_file_name);
    fprintf(out, "Architecture Scope       : Stages 1 through 8 (End-to-End Complete Pipeline)\n\n");

    fprintf(out, "Pipeline Execution Stages:\n");
    fprintf(out, "  [Level 1: Structure]\n");
    fprintf(out, "    Stage 1: JSON Banking Input       : %s\n",
            stage_display_status(g_diagnostic_catalog.stage_statuses[STAGE_INPUT]));
    fprintf(out, "    Stage 2: Lexical Analyzer (Flex)  : %s\n",
            stage_display_status(g_diagnostic_catalog.stage_statuses[STAGE_LEXER]));
    fprintf(out, "    Stage 3: Syntax Analyzer (Bison)  : %s\n",
            stage_display_status(g_diagnostic_catalog.stage_statuses[STAGE_PARSER]));
    fprintf(out, "  [Level 2: Banking Data Validity]\n");
    fprintf(out, "    Stage 4: Semantic Analyzer        : %s\n",
            stage_display_status(g_diagnostic_catalog.stage_statuses[STAGE_SEMANTIC]));
    fprintf(out, "    Stage 5A: Pattern Matching Engine : %s\n",
            stage_display_status(g_diagnostic_catalog.stage_statuses[STAGE_PATTERN]));
    fprintf(out, "    Stage 5B: Domain Rule Engine      : %s\n",
            stage_display_status(g_diagnostic_catalog.stage_statuses[STAGE_DOMAIN]));
    fprintf(out, "  [Level 3: Inter-Field & Diagnostics]\n");
    fprintf(out, "    Stage 6: Cross-Field Validation   : %s\n",
            stage_display_status(g_diagnostic_catalog.stage_statuses[STAGE_CROSS_FIELD]));
    fprintf(out, "    Stage 7: Error Aggregation Engine : %s (%d error(s) recorded)\n",
            stage_display_status(g_diagnostic_catalog.stage_statuses[STAGE_ERROR_AGGREGATOR]),
            g_diagnostic_catalog.error_count);
    fprintf(out, "    Stage 8: Final Validation Report  : GENERATED (%s)\n",
            overall_valid ? "VERIFIED VALID" : "REJECTED INVALID");

    if (g_diagnostic_catalog.stage_statuses[STAGE_LEXER] == STAGE_STATUS_PASS) {
        fprintf(out, "\nLexical Token Summary:\n");
        fprintf(out, "  Strings (\"...\")        : %lu\n", g_diagnostic_catalog.token_stats.strings);
        fprintf(out, "  Numbers                : %lu\n", g_diagnostic_catalog.token_stats.numbers);
        fprintf(out, "  Boolean Literals       : %lu\n", g_diagnostic_catalog.token_stats.booleans);
        fprintf(out, "  Null Literals          : %lu\n", g_diagnostic_catalog.token_stats.nulls);
        fprintf(out, "  Braces ('{' / '}')     : %lu / %lu\n",
                g_diagnostic_catalog.token_stats.lbrace, g_diagnostic_catalog.token_stats.rbrace);
        fprintf(out, "  Brackets ('[' / ']')   : %lu / %lu\n",
                g_diagnostic_catalog.token_stats.lbracket, g_diagnostic_catalog.token_stats.rbracket);
        fprintf(out, "  Colons (':')           : %lu\n", g_diagnostic_catalog.token_stats.colons);
        fprintf(out, "  Commas (',')           : %lu\n", g_diagnostic_catalog.token_stats.commas);
        fprintf(out, "  Total Meaningful Tokens: %lu\n", g_diagnostic_catalog.token_stats.total);
    }

    if (g_diagnostic_catalog.stage_statuses[STAGE_PARSER] == STAGE_STATUS_PASS) {
        fprintf(out, "\nField Validation Summary:\n");
        for (int i = 0; i < NUM_CORE_FIELDS; i++) {
            FieldValidationSummary *f = &g_diagnostic_catalog.field_summaries[i];
            fprintf(out, "  %-22s : %-7s", f->name, field_status_name(f->status));
            if (f->status == FIELD_STATUS_FAIL && f->failure_reason[0] != '\0') {
                fprintf(out, " (%s)", f->failure_reason);
            }
            fprintf(out, "\n");
        }
    }

    if (g_diagnostic_catalog.error_count > 0) {
        fprintf(out, "\nErrors Detected in Pipeline (%d):\n", g_diagnostic_catalog.error_count);
        int idx = 1;
        ValidationError *curr = g_diagnostic_catalog.diagnostics_head;
        while (curr) {
            if (curr->severity == SEVERITY_ERROR) {
                fprintf(out, "  %2d. [%s] ", idx++, pipeline_stage_name(curr->stage));
                if (curr->rule_id && strlen(curr->rule_id) > 0) {
                    fprintf(out, "[%s] ", curr->rule_id);
                }
                if (curr->field_name && strlen(curr->field_name) > 0) {
                    fprintf(out, "Field '%s': ", curr->field_name);
                }
                if (curr->line > 0) {
                    fprintf(out, "(Line %d", curr->line);
                    if (curr->col > 0) fprintf(out, ", Col %d", curr->col);
                    fprintf(out, ") ");
                }
                fprintf(out, "%s\n", curr->message);
            }
            curr = curr->next;
        }
    }

    if (g_diagnostic_catalog.warning_count > 0) {
        fprintf(out, "\nWarnings (%d):\n", g_diagnostic_catalog.warning_count);
        int idx = 1;
        ValidationError *curr = g_diagnostic_catalog.diagnostics_head;
        while (curr) {
            if (curr->severity == SEVERITY_WARNING) {
                fprintf(out, "  %2d. [WARNING] [%s] ", idx++, pipeline_stage_name(curr->stage));
                if (curr->rule_id && strlen(curr->rule_id) > 0) {
                    fprintf(out, "[%s] ", curr->rule_id);
                }
                if (curr->field_name && strlen(curr->field_name) > 0) {
                    fprintf(out, "Field '%s': ", curr->field_name);
                }
                if (curr->line > 0) {
                    fprintf(out, "(Line %d", curr->line);
                    if (curr->col > 0) fprintf(out, ", Col %d", curr->col);
                    fprintf(out, ") ");
                }
                fprintf(out, "%s\n", curr->message);
            }
            curr = curr->next;
        }
    }

    fprintf(out, "\nSummary Statistics:\n");
    fprintf(out, "  Total Errors   : %d\n", g_diagnostic_catalog.error_count);
    fprintf(out, "  Total Warnings : %d\n", g_diagnostic_catalog.warning_count);
    fprintf(out, "  Stages Passed  : %d\n", stages_passed);
    fprintf(out, "  Stages Failed  : %d\n", stages_failed);

    fprintf(out, "\nFINAL RESULT: %s\n", overall_valid ? "VALID BANKING DATA" : "INVALID BANKING DATA");
    fprintf(out, "AUDIT DECISION: %s\n", overall_valid ? "APPROVED FOR INGESTION" : "REJECTED (NON-COMPLIANT DATA)");
    fprintf(out, "========================================================================\n\n");
}

static void print_json_report(FILE *out) {
    int stages_passed = 0;
    int stages_failed = 0;

    for (int s = 1; s <= 8; s++) {
        if (g_diagnostic_catalog.stage_statuses[s] == STAGE_STATUS_PASS) {
            stages_passed++;
        } else if (g_diagnostic_catalog.stage_statuses[s] == STAGE_STATUS_FAIL) {
            stages_failed++;
        }
    }

    bool overall_valid = (g_diagnostic_catalog.error_count == 0 && stages_failed == 0);
    error_reporter_set_stage_status(STAGE_FINAL_REPORT, STAGE_STATUS_PASS);
    stages_passed++;

    fprintf(out, "{\n");
    fprintf(out, "  \"input_file\": ");
    print_json_escaped_string(out, g_diagnostic_catalog.input_file_name);
    fprintf(out, ",\n  \"overall_valid\": %s,\n", overall_valid ? "true" : "false");
    fprintf(out, "  \"final_result\": ");
    print_json_escaped_string(out, overall_valid ? "VALID BANKING DATA" : "INVALID BANKING DATA");
    fprintf(out, ",\n  \"audit_decision\": ");
    print_json_escaped_string(out, overall_valid ? "APPROVED FOR INGESTION" : "REJECTED (NON-COMPLIANT DATA)");
    fprintf(out, ",\n  \"stages\": {\n");

    for (int s = 1; s <= NUM_PIPELINE_STAGES; s++) {
        fprintf(out, "    \"%s\": \"%s\"%s\n",
                pipeline_stage_label((PipelineStage)s),
                stage_display_status(g_diagnostic_catalog.stage_statuses[s]),
                (s == NUM_PIPELINE_STAGES) ? "" : ",");
    }
    fprintf(out, "  },\n");

    fprintf(out, "  \"summary\": {\n");
    fprintf(out, "    \"stages_passed\": %d,\n", stages_passed);
    fprintf(out, "    \"stages_failed\": %d,\n", stages_failed);
    fprintf(out, "    \"total_errors\": %d,\n", g_diagnostic_catalog.error_count);
    fprintf(out, "    \"total_warnings\": %d\n", g_diagnostic_catalog.warning_count);
    fprintf(out, "  },\n");

    fprintf(out, "  \"token_statistics\": {\n");
    fprintf(out, "    \"strings\": %lu,\n", g_diagnostic_catalog.token_stats.strings);
    fprintf(out, "    \"numbers\": %lu,\n", g_diagnostic_catalog.token_stats.numbers);
    fprintf(out, "    \"booleans\": %lu,\n", g_diagnostic_catalog.token_stats.booleans);
    fprintf(out, "    \"nulls\": %lu,\n", g_diagnostic_catalog.token_stats.nulls);
    fprintf(out, "    \"braces_open\": %lu,\n", g_diagnostic_catalog.token_stats.lbrace);
    fprintf(out, "    \"braces_close\": %lu,\n", g_diagnostic_catalog.token_stats.rbrace);
    fprintf(out, "    \"brackets_open\": %lu,\n", g_diagnostic_catalog.token_stats.lbracket);
    fprintf(out, "    \"brackets_close\": %lu,\n", g_diagnostic_catalog.token_stats.rbracket);
    fprintf(out, "    \"colons\": %lu,\n", g_diagnostic_catalog.token_stats.colons);
    fprintf(out, "    \"commas\": %lu,\n", g_diagnostic_catalog.token_stats.commas);
    fprintf(out, "    \"total_tokens\": %lu\n", g_diagnostic_catalog.token_stats.total);
    fprintf(out, "  },\n");

    fprintf(out, "  \"fields\": {\n");
    for (int i = 0; i < NUM_CORE_FIELDS; i++) {
        FieldValidationSummary *f = &g_diagnostic_catalog.field_summaries[i];
        fprintf(out, "    \"%s\": {\n", f->name);
        fprintf(out, "      \"status\": \"%s\",\n", field_status_name(f->status));
        fprintf(out, "      \"failure_reason\": ");
        if (f->status == FIELD_STATUS_FAIL && f->failure_reason[0] != '\0') {
            print_json_escaped_string(out, f->failure_reason);
        } else {
            fputs("null", out);
        }
        fprintf(out, "\n    }%s\n", (i == NUM_CORE_FIELDS - 1) ? "" : ",");
    }
    fprintf(out, "  },\n");

    fprintf(out, "  \"diagnostics\": [\n");
    ValidationError *curr = g_diagnostic_catalog.diagnostics_head;
    while (curr) {
        fprintf(out, "    {\n");
        fprintf(out, "      \"stage\": \"%s\",\n", pipeline_stage_label(curr->stage));
        fprintf(out, "      \"severity\": \"%s\",\n", diagnostic_severity_name(curr->severity));
        fprintf(out, "      \"rule_id\": ");
        print_json_escaped_string(out, curr->rule_id);
        fprintf(out, ",\n      \"field\": ");
        print_json_escaped_string(out, curr->field_name);
        fprintf(out, ",\n      \"line\": %d,\n", curr->line);
        fprintf(out, "      \"col\": %d,\n", curr->col);
        fprintf(out, "      \"message\": ");
        print_json_escaped_string(out, curr->message);
        fprintf(out, "\n    }%s\n", curr->next ? "," : "");
        curr = curr->next;
    }
    fprintf(out, "  ]\n");
    fprintf(out, "}\n");
}

void run_stage8_validation_report(bool report_json_mode, FILE *out) {
    if (!out) out = stdout;

    if (report_json_mode) {
        print_json_report(out);
    } else {
        print_text_report(out);
    }
}
