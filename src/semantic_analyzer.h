#ifndef SEMANTIC_ANALYZER_H
#define SEMANTIC_ANALYZER_H

#include "../include/common.h"
#include "json_value.h"

/* Stage 4: Semantic Analysis (Structure, 10/10 Schema Presence, Data Types, Blank-Field Checks, Duplicate Keys) */
bool run_stage4_semantic_analysis(const JsonValue *root);

/* Stage 5A: Pattern Matching Engine (Regex validation of customer_id, full_name, email, phone, dob, pan, acc_no, ifsc) */
bool run_stage5a_pattern_matching(const JsonValue *root);

/* Stage 5B: Domain Rule Engine (account_type and deposit limits) */
bool run_stage5b_domain_rules(const JsonValue *root);

/* Combined pipeline wrapper for Stages 4, 5A, and 5B */
bool run_semantic_analysis(const JsonValue *root);

#endif /* SEMANTIC_ANALYZER_H */
