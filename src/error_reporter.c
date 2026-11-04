#include "error_reporter.h"

extern bool g_report_json_mode;
#define STAGE_PRINT(...) do { if (!g_report_json_mode) printf(__VA_ARGS__); } while (0)

DiagnosticCatalog g_diagnostic_catalog;

static const char *g_core_field_names[NUM_CORE_FIELDS] = {
    FIELD_CUSTOMER_ID,
    FIELD_FULL_NAME,
    FIELD_EMAIL,
    FIELD_PHONE,
    FIELD_DOB,
    FIELD_PAN,
    FIELD_ACCOUNT_NO,
    FIELD_IFSC,
    FIELD_ACCOUNT_TYPE,
    FIELD_INITIAL_DEPOSIT
};

void error_reporter_init(const char *input_file_name) {
    memset(&g_diagnostic_catalog, 0, sizeof(DiagnosticCatalog));
    g_diagnostic_catalog.input_file_name = input_file_name ? input_file_name : "standard_input";

    for (int s = 0; s <= NUM_PIPELINE_STAGES; s++) {
        g_diagnostic_catalog.stage_statuses[s] = STAGE_STATUS_NOT_RUN;
    }

    for (int i = 0; i < NUM_CORE_FIELDS; i++) {
        strncpy(g_diagnostic_catalog.field_summaries[i].name, g_core_field_names[i],
                sizeof(g_diagnostic_catalog.field_summaries[i].name) - 1);
        g_diagnostic_catalog.field_summaries[i].name[sizeof(g_diagnostic_catalog.field_summaries[i].name) - 1] = '\0';
        g_diagnostic_catalog.field_summaries[i].status = FIELD_STATUS_UNKNOWN;
        g_diagnostic_catalog.field_summaries[i].failure_reason[0] = '\0';
    }

    g_diagnostic_catalog.diagnostics_head = NULL;
    g_diagnostic_catalog.diagnostics_tail = NULL;
    g_diagnostic_catalog.error_count = 0;
    g_diagnostic_catalog.warning_count = 0;
}

void error_reporter_cleanup(void) {
    ValidationError *curr = g_diagnostic_catalog.diagnostics_head;
    while (curr) {
        ValidationError *tmp = curr->next;
        if (curr->rule_id) free(curr->rule_id);
        if (curr->field_name) free(curr->field_name);
        if (curr->message) free(curr->message);
        free(curr);
        curr = tmp;
    }
    g_diagnostic_catalog.diagnostics_head = NULL;
    g_diagnostic_catalog.diagnostics_tail = NULL;
    g_diagnostic_catalog.error_count = 0;
    g_diagnostic_catalog.warning_count = 0;
}

void error_reporter_add_diagnostic(PipelineStage stage, DiagnosticSeverity severity,
                                  const char *rule_id, const char *field_name,
                                  int line, int col, const char *message) {
    if (!message) message = "Unspecified diagnostic";

    /* Deduplication check: Avoid adding duplicate diagnostic for identical defect */
    ValidationError *check = g_diagnostic_catalog.diagnostics_head;
    while (check) {
        if (check->severity == severity &&
            ((!check->field_name && !field_name) ||
             (check->field_name && field_name && strcmp(check->field_name, field_name) == 0)) &&
            strcmp(check->message, message) == 0) {
            return; /* Duplicate found, do not add */
        }
        check = check->next;
    }

    ValidationError *diag = (ValidationError *)malloc(sizeof(ValidationError));
    if (!diag) {
        fprintf(stderr, "Fatal error: out of memory in error_reporter_add_diagnostic\n");
        exit(EXIT_FAILURE);
    }
    diag->stage = stage;
    diag->severity = severity;
    diag->rule_id = rule_id ? strdup(rule_id) : NULL;
    diag->field_name = (field_name && field_name[0] != '\0') ? strdup(field_name) : NULL;
    diag->message = strdup(message);
    diag->line = line;
    diag->col = col;
    diag->next = NULL;

    if (!g_diagnostic_catalog.diagnostics_head) {
        g_diagnostic_catalog.diagnostics_head = diag;
        g_diagnostic_catalog.diagnostics_tail = diag;
    } else {
        g_diagnostic_catalog.diagnostics_tail->next = diag;
        g_diagnostic_catalog.diagnostics_tail = diag;
    }

    if (severity == SEVERITY_ERROR) {
        g_diagnostic_catalog.error_count++;
    } else {
        g_diagnostic_catalog.warning_count++;
    }
}

void error_reporter_add_error(PipelineStage stage, const char *rule_id,
                              const char *field_name, int line, int col, const char *message) {
    error_reporter_add_diagnostic(stage, SEVERITY_ERROR, rule_id, field_name, line, col, message);
}

void error_reporter_add_warning(PipelineStage stage, const char *rule_id,
                                const char *field_name, int line, int col, const char *message) {
    error_reporter_add_diagnostic(stage, SEVERITY_WARNING, rule_id, field_name, line, col, message);
}

void error_reporter_set_stage_status(PipelineStage stage, StageStatus status) {
    if (stage >= 1 && stage <= NUM_PIPELINE_STAGES) {
        g_diagnostic_catalog.stage_statuses[stage] = status;
    }
}

StageStatus error_reporter_get_stage_status(PipelineStage stage) {
    if (stage >= 1 && stage <= NUM_PIPELINE_STAGES) {
        return g_diagnostic_catalog.stage_statuses[stage];
    }
    return STAGE_STATUS_NOT_RUN;
}

void error_reporter_set_field_status(const char *field_name, FieldStatus status, const char *reason) {
    if (!field_name) return;
    for (int i = 0; i < NUM_CORE_FIELDS; i++) {
        if (strcmp(g_diagnostic_catalog.field_summaries[i].name, field_name) == 0) {
            g_diagnostic_catalog.field_summaries[i].status = status;
            if (reason) {
                strncpy(g_diagnostic_catalog.field_summaries[i].failure_reason, reason,
                        sizeof(g_diagnostic_catalog.field_summaries[i].failure_reason) - 1);
                g_diagnostic_catalog.field_summaries[i].failure_reason[sizeof(g_diagnostic_catalog.field_summaries[i].failure_reason) - 1] = '\0';
            }
            break;
        }
    }
}

void error_reporter_sync_tokens(int lbrace, int rbrace, int lbracket, int rbracket,
                                int colon, int comma, int str_cnt, int num_cnt,
                                int bool_cnt, int null_cnt, int total) {
    g_diagnostic_catalog.token_stats.lbrace = lbrace;
    g_diagnostic_catalog.token_stats.rbrace = rbrace;
    g_diagnostic_catalog.token_stats.lbracket = lbracket;
    g_diagnostic_catalog.token_stats.rbracket = rbracket;
    g_diagnostic_catalog.token_stats.colons = colon;
    g_diagnostic_catalog.token_stats.commas = comma;
    g_diagnostic_catalog.token_stats.strings = str_cnt;
    g_diagnostic_catalog.token_stats.numbers = num_cnt;
    g_diagnostic_catalog.token_stats.booleans = bool_cnt;
    g_diagnostic_catalog.token_stats.nulls = null_cnt;
    g_diagnostic_catalog.token_stats.total = total;
}

const char *pipeline_stage_name(PipelineStage stage) {
    switch (stage) {
        case STAGE_INPUT:            return "JSON Banking Input";
        case STAGE_LEXER:            return "Lexical Analyzer";
        case STAGE_PARSER:           return "Syntax Analyzer";
        case STAGE_SEMANTIC:         return "Semantic Analyzer";
        case STAGE_PATTERN:          return "Pattern Matching Engine";
        case STAGE_DOMAIN:           return "Domain Rule Engine";
        case STAGE_CROSS_FIELD:      return "Cross-Field Validation";
        case STAGE_ERROR_AGGREGATOR: return "Error Detection and Aggregation";
        case STAGE_FINAL_REPORT:     return "Validation Report";
        default:                     return "Unknown Stage";
    }
}

const char *pipeline_stage_label(PipelineStage stage) {
    switch (stage) {
        case STAGE_INPUT:            return "stage1_input";
        case STAGE_LEXER:            return "stage2_lexer";
        case STAGE_PARSER:           return "stage3_parser";
        case STAGE_SEMANTIC:         return "stage4_semantic";
        case STAGE_PATTERN:          return "stage5a_pattern";
        case STAGE_DOMAIN:           return "stage5b_domain";
        case STAGE_CROSS_FIELD:      return "stage6_cross_field";
        case STAGE_ERROR_AGGREGATOR: return "stage7_error_aggregation";
        case STAGE_FINAL_REPORT:     return "stage8_validation_report";
        default:                     return "stage_unknown";
    }
}

const char *diagnostic_severity_name(DiagnosticSeverity sev) {
    switch (sev) {
        case SEVERITY_ERROR:   return "ERROR";
        case SEVERITY_WARNING: return "WARNING";
        default:               return "INFO";
    }
}

const char *field_status_name(FieldStatus status) {
    switch (status) {
        case FIELD_STATUS_PASS:    return "PASS";
        case FIELD_STATUS_FAIL:    return "FAIL";
        case FIELD_STATUS_MISSING: return "MISSING";
        default:                   return "NOT CHECKED";
    }
}

const char *stage_status_name(StageStatus status) {
    switch (status) {
        case STAGE_STATUS_PASS:    return "PASS";
        case STAGE_STATUS_FAIL:    return "FAIL";
        default:                   return "NOT EXECUTED";
    }
}

void run_stage7_error_aggregation(void) {
    if (g_diagnostic_catalog.error_count == 0) {
        STAGE_PRINT("Error Detection and Aggregation: PASS\n");
        STAGE_PRINT("  Total errors recorded: 0\n");
        if (g_diagnostic_catalog.warning_count > 0) {
            STAGE_PRINT("  Total warnings recorded: %d\n", g_diagnostic_catalog.warning_count);
        }
        STAGE_PRINT("  Diagnostic catalog: CLEAN (No violations detected)\n");
        error_reporter_set_stage_status(STAGE_ERROR_AGGREGATOR, STAGE_STATUS_PASS);
    } else {
        STAGE_PRINT("Error Detection and Aggregation: FAIL\n");
        STAGE_PRINT("  Total errors recorded: %d\n", g_diagnostic_catalog.error_count);
        if (g_diagnostic_catalog.warning_count > 0) {
            STAGE_PRINT("  Total warnings recorded: %d\n", g_diagnostic_catalog.warning_count);
        }
        STAGE_PRINT("  Diagnostic Classification Buffer:\n");
        int idx = 1;
        ValidationError *curr = g_diagnostic_catalog.diagnostics_head;
        while (curr) {
            STAGE_PRINT("    %d. [%s] [%s] ", idx++,
                        (curr->severity == SEVERITY_ERROR) ? "ERROR" : "WARNING",
                        pipeline_stage_name(curr->stage));
            if (curr->field_name && strlen(curr->field_name) > 0) {
                STAGE_PRINT("Field '%s': ", curr->field_name);
            }
            if (curr->line > 0) {
                STAGE_PRINT("(Line %d, Col %d) ", curr->line, curr->col);
            }
            STAGE_PRINT("%s", curr->message);
            if (curr->rule_id && strlen(curr->rule_id) > 0) {
                STAGE_PRINT(" [%s]", curr->rule_id);
            }
            STAGE_PRINT("\n");
            curr = curr->next;
        }
        error_reporter_set_stage_status(STAGE_ERROR_AGGREGATOR, STAGE_STATUS_FAIL);
    }
}
