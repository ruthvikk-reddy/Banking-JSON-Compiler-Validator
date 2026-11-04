#ifndef ERROR_REPORTER_H
#define ERROR_REPORTER_H

#include "../include/common.h"

typedef enum {
    SEVERITY_ERROR,
    SEVERITY_WARNING
} DiagnosticSeverity;

typedef enum {
    STAGE_NONE = 0,
    STAGE_INPUT = 1,            /* Stage 1: JSON Banking Input */
    STAGE_LEXER = 2,            /* Stage 2: Lexical Analyzer */
    STAGE_PARSER = 3,           /* Stage 3: Syntax Analyzer */
    STAGE_SEMANTIC = 4,         /* Stage 4: Semantic Analyzer */
    STAGE_PATTERN = 5,          /* Stage 5A: Pattern Matching Engine */
    STAGE_DOMAIN = 6,           /* Stage 5B: Domain Rule Engine */
    STAGE_CROSS_FIELD = 7,      /* Stage 6: Cross-Field Validation Engine */
    STAGE_ERROR_AGGREGATOR = 8, /* Stage 7: Error Detection and Aggregation */
    STAGE_FINAL_REPORT = 9      /* Stage 8: Validation Report / Final Output */
} PipelineStage;

#define NUM_PIPELINE_STAGES 9

typedef enum {
    STAGE_STATUS_NOT_RUN,
    STAGE_STATUS_PASS,
    STAGE_STATUS_FAIL
} StageStatus;

typedef enum {
    FIELD_STATUS_UNKNOWN,
    FIELD_STATUS_PASS,
    FIELD_STATUS_FAIL,
    FIELD_STATUS_MISSING
} FieldStatus;

typedef struct ValidationError {
    PipelineStage stage;
    DiagnosticSeverity severity;
    char *rule_id;
    char *field_name;
    char *message;
    int line;
    int col;
    struct ValidationError *next;
} ValidationError;

typedef struct {
    char name[32];
    FieldStatus status;
    char failure_reason[256];
} FieldValidationSummary;

typedef struct {
    unsigned long strings;
    unsigned long numbers;
    unsigned long booleans;
    unsigned long nulls;
    unsigned long lbrace;
    unsigned long rbrace;
    unsigned long lbracket;
    unsigned long rbracket;
    unsigned long colons;
    unsigned long commas;
    unsigned long total;
} TokenStats;

typedef struct {
    const char *input_file_name;
    StageStatus stage_statuses[NUM_PIPELINE_STAGES + 1];
    FieldValidationSummary field_summaries[NUM_CORE_FIELDS];
    TokenStats token_stats;

    ValidationError *diagnostics_head;
    ValidationError *diagnostics_tail;
    int error_count;
    int warning_count;
} DiagnosticCatalog;

extern DiagnosticCatalog g_diagnostic_catalog;

/* Initialization and Memory Cleanup */
void error_reporter_init(const char *input_file_name);
void error_reporter_cleanup(void);

/* Diagnostic Registration with Deduplication */
void error_reporter_add_diagnostic(PipelineStage stage, DiagnosticSeverity severity,
                                  const char *rule_id, const char *field_name,
                                  int line, int col, const char *message);

void error_reporter_add_error(PipelineStage stage, const char *rule_id,
                              const char *field_name, int line, int col, const char *message);
void error_reporter_add_warning(PipelineStage stage, const char *rule_id,
                                const char *field_name, int line, int col, const char *message);

/* Stage and Field Status Tracking */
void error_reporter_set_stage_status(PipelineStage stage, StageStatus status);
StageStatus error_reporter_get_stage_status(PipelineStage stage);
void error_reporter_set_field_status(const char *field_name, FieldStatus status, const char *reason);

/* Token Statistics Recording */
void error_reporter_sync_tokens(int lbrace, int rbrace, int lbracket, int rbracket,
                                int colon, int comma, int str_cnt, int num_cnt,
                                int bool_cnt, int null_cnt, int total);

/* Names and Labels */
const char *pipeline_stage_name(PipelineStage stage);
const char *pipeline_stage_label(PipelineStage stage);
const char *diagnostic_severity_name(DiagnosticSeverity sev);
const char *field_status_name(FieldStatus status);
const char *stage_status_name(StageStatus status);

/* Stage 7 Execution */
void run_stage7_error_aggregation(void);

#endif /* ERROR_REPORTER_H */
