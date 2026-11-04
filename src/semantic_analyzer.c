#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include "semantic_analyzer.h"
#include "pattern_matcher.h"
#include "domain_rules.h"
#include "error_reporter.h"

extern bool g_report_json_mode;
#define STAGE_PRINT(...) do { if (!g_report_json_mode) printf(__VA_ARGS__); } while (0)

static bool is_blank_string(const char *str) {
    if (!str || str[0] == '\0') return true;
    for (int i = 0; str[i] != '\0'; i++) {
        if (!isspace((unsigned char)str[i])) return false;
    }
    return true;
}

static bool is_core_field(const char *key) {
    if (!key) return false;
    static const char *core_keys[NUM_CORE_FIELDS] = {
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
    for (int i = 0; i < NUM_CORE_FIELDS; i++) {
        if (strcmp(key, core_keys[i]) == 0) return true;
    }
    return false;
}

/* Stage 4: Semantic Analysis */
bool run_stage4_semantic_analysis(const JsonValue *root) {
    if (!root) {
        STAGE_PRINT("Semantic Analyzer: FAIL\n");
        STAGE_PRINT("  Semantic Analyzer rejects it: Empty or NULL parse tree.\n");
        error_reporter_add_error(STAGE_SEMANTIC, "RULE_STRUCTURE", NULL, 0, 0, "Empty or NULL parse tree.");
        error_reporter_set_stage_status(STAGE_SEMANTIC, STAGE_STATUS_FAIL);
        return false;
    }

    /* 1. Structural check: Top-level must be JSON_OBJECT */
    if (root->type != JSON_OBJECT) {
        STAGE_PRINT("Semantic Analyzer: FAIL\n");
        STAGE_PRINT("  Semantic Analyzer rejects it: Top-level JSON value is a %s. Banking record must be a JSON Object.\n",
                    json_type_name(root->type));
        char err_msg[256];
        snprintf(err_msg, sizeof(err_msg), "Top-level JSON value is a %s. Banking record must be a JSON Object.",
                 json_type_name(root->type));
        error_reporter_add_error(STAGE_SEMANTIC, "RULE_TOP_LEVEL_OBJECT", NULL, root->line, root->col, err_msg);
        error_reporter_set_stage_status(STAGE_SEMANTIC, STAGE_STATUS_FAIL);
        return false;
    }

    char errors[32][256];
    int error_count = 0;

    /* 2. Check for duplicate keys in top-level object */
    JsonObjectMember *curr = root->data.object_val.head;
    while (curr) {
        bool already_seen = false;
        JsonObjectMember *prev = root->data.object_val.head;
        while (prev != curr) {
            if (prev->key && curr->key && strcmp(prev->key, curr->key) == 0) {
                already_seen = true;
                break;
            }
            prev = prev->next;
        }
        if (already_seen && error_count < 32) {
            snprintf(errors[error_count], sizeof(errors[0]),
                     "Semantic Analyzer rejects it: Duplicate key '%s' found in banking record object.", curr->key);
            error_reporter_add_error(STAGE_SEMANTIC, "RULE_UNIQUE_KEYS", curr->key,
                                     curr->line, curr->col, errors[error_count]);
            error_count++;
        }
        curr = curr->next;
    }

    /* 3. Check for unknown fields (warnings) */
    JsonObjectMember *m = root->data.object_val.head;
    while (m) {
        if (m->key && !is_core_field(m->key)) {
            STAGE_PRINT("  Semantic Analyzer: Unknown field '%s' detected (not part of the standard 10-field banking schema).\n", m->key);
            char warn_msg[256];
            snprintf(warn_msg, sizeof(warn_msg),
                     "Unknown field '%s' detected (not part of the standard 10-field banking schema).", m->key);
            error_reporter_add_warning(STAGE_SEMANTIC, "RULE_UNKNOWN_FIELD", m->key,
                                       m->line, m->col, warn_msg);
        }
        m = m->next;
    }

    /* 4. Check presence and types of the 10 Core Fields */
    static const char *string_core_fields[] = {
        FIELD_CUSTOMER_ID,
        FIELD_FULL_NAME,
        FIELD_EMAIL,
        FIELD_PHONE,
        FIELD_DOB,
        FIELD_PAN,
        FIELD_ACCOUNT_NO,
        FIELD_IFSC,
        FIELD_ACCOUNT_TYPE
    };
    int num_str_fields = sizeof(string_core_fields) / sizeof(string_core_fields[0]);

    int found_fields = 0;

    /* Check 9 string fields */
    for (int i = 0; i < num_str_fields; i++) {
        const char *fname = string_core_fields[i];
        JsonObjectMember *mem = json_object_find_member(root, fname);
        if (!mem) {
            if (error_count < 32) {
                snprintf(errors[error_count], sizeof(errors[0]),
                         "Semantic Analyzer rejects it: Mandatory field '%s' is missing (MISSING FIELD).", fname);
                error_reporter_add_error(STAGE_SEMANTIC, "RULE_MANDATORY_FIELD", fname,
                                         0, 0, errors[error_count]);
                error_reporter_set_field_status(fname, FIELD_STATUS_MISSING, "Mandatory field is missing");
                error_count++;
            }
            continue;
        }

        found_fields++;

        if (mem->value->type != JSON_STRING) {
            if (error_count < 32) {
                snprintf(errors[error_count], sizeof(errors[0]),
                         "Semantic Analyzer rejects it: TYPE ERROR: Field '%s': Expected JSON String, but found %s.",
                         fname, json_type_name(mem->value->type));
                error_reporter_add_error(STAGE_SEMANTIC, "RULE_DATA_TYPE", fname,
                                         mem->value->line, mem->value->col, errors[error_count]);
                char rsn[128];
                snprintf(rsn, sizeof(rsn), "Expected JSON String, found %s", json_type_name(mem->value->type));
                error_reporter_set_field_status(fname, FIELD_STATUS_FAIL, rsn);
                error_count++;
            }
            continue;
        }

        const char *sval = mem->value->data.string_val;
        if (is_blank_string(sval)) {
            if (sval[0] == '\0') {
                if (error_count < 32) {
                    snprintf(errors[error_count], sizeof(errors[0]),
                             "Semantic Analyzer rejects it: Field '%s' cannot be empty.", fname);
                    error_reporter_add_error(STAGE_SEMANTIC, "RULE_EMPTY_FIELD", fname,
                                             mem->value->line, mem->value->col, errors[error_count]);
                    error_reporter_set_field_status(fname, FIELD_STATUS_FAIL, "Empty string value");
                    error_count++;
                }
            } else {
                if (error_count < 32) {
                    snprintf(errors[error_count], sizeof(errors[0]),
                             "Semantic Analyzer rejects it: Field '%s' cannot be blank or contain only whitespace.", fname);
                    error_reporter_add_error(STAGE_SEMANTIC, "RULE_BLANK_FIELD", fname,
                                             mem->value->line, mem->value->col, errors[error_count]);
                    error_reporter_set_field_status(fname, FIELD_STATUS_FAIL, "Blank or whitespace-only value");
                    error_count++;
                }
            }
            continue;
        }

        error_reporter_set_field_status(fname, FIELD_STATUS_PASS, NULL);
    }

    /* Check initial_deposit */
    JsonObjectMember *deposit_mem = json_object_find_member(root, FIELD_INITIAL_DEPOSIT);
    if (!deposit_mem) {
        if (error_count < 32) {
            snprintf(errors[error_count], sizeof(errors[0]),
                     "Semantic Analyzer rejects it: Mandatory field '%s' is missing (MISSING FIELD).", FIELD_INITIAL_DEPOSIT);
            error_reporter_add_error(STAGE_SEMANTIC, "RULE_MANDATORY_FIELD", FIELD_INITIAL_DEPOSIT,
                                     0, 0, errors[error_count]);
            error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_MISSING, "Mandatory field is missing");
            error_count++;
        }
    } else {
        found_fields++;
        if (deposit_mem->value->type != JSON_NUMBER) {
            if (deposit_mem->value->type == JSON_STRING) {
                const char *dep_str = deposit_mem->value->data.string_val;
                if (is_blank_string(dep_str)) {
                    if (dep_str[0] == '\0') {
                        if (error_count < 32) {
                            snprintf(errors[error_count], sizeof(errors[0]),
                                     "Semantic Analyzer rejects it: TYPE ERROR: Field '%s': Expected JSON Number, but found empty String.",
                                     FIELD_INITIAL_DEPOSIT);
                            error_reporter_add_error(STAGE_SEMANTIC, "RULE_DATA_TYPE", FIELD_INITIAL_DEPOSIT,
                                                     deposit_mem->value->line, deposit_mem->value->col, errors[error_count]);
                            error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_FAIL, "Found empty String");
                            error_count++;
                        }
                    } else {
                        if (error_count < 32) {
                            snprintf(errors[error_count], sizeof(errors[0]),
                                     "Semantic Analyzer rejects it: TYPE ERROR: Field '%s': Expected JSON Number, but found whitespace-only String.",
                                     FIELD_INITIAL_DEPOSIT);
                            error_reporter_add_error(STAGE_SEMANTIC, "RULE_DATA_TYPE", FIELD_INITIAL_DEPOSIT,
                                                     deposit_mem->value->line, deposit_mem->value->col, errors[error_count]);
                            error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_FAIL, "Found whitespace-only String");
                            error_count++;
                        }
                    }
                } else {
                    if (error_count < 32) {
                        snprintf(errors[error_count], sizeof(errors[0]),
                                 "Semantic Analyzer rejects it: TYPE ERROR: Field '%s': Expected JSON Number, but found String.",
                                 FIELD_INITIAL_DEPOSIT);
                        error_reporter_add_error(STAGE_SEMANTIC, "RULE_DATA_TYPE", FIELD_INITIAL_DEPOSIT,
                                                 deposit_mem->value->line, deposit_mem->value->col, errors[error_count]);
                        error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_FAIL, "Expected JSON Number, found String");
                        error_count++;
                    }
                }
            } else {
                if (error_count < 32) {
                    snprintf(errors[error_count], sizeof(errors[0]),
                             "Semantic Analyzer rejects it: TYPE ERROR: Field '%s': Expected JSON Number, but found %s.",
                             FIELD_INITIAL_DEPOSIT, json_type_name(deposit_mem->value->type));
                    error_reporter_add_error(STAGE_SEMANTIC, "RULE_DATA_TYPE", FIELD_INITIAL_DEPOSIT,
                                             deposit_mem->value->line, deposit_mem->value->col, errors[error_count]);
                    char rsn[128];
                    snprintf(rsn, sizeof(rsn), "Expected JSON Number, found %s", json_type_name(deposit_mem->value->type));
                    error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_FAIL, rsn);
                    error_count++;
                }
            }
        } else {
            error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_PASS, NULL);
        }
    }

    if (error_count == 0) {
        STAGE_PRINT("Semantic Analyzer: PASS\n");
        STAGE_PRINT("  Required fields: 10/10\n");
        STAGE_PRINT("  Data types: PASS\n");
        STAGE_PRINT("  Blank-field checks: PASS\n");
        error_reporter_set_stage_status(STAGE_SEMANTIC, STAGE_STATUS_PASS);
        return true;
    } else {
        STAGE_PRINT("Semantic Analyzer: FAIL\n");
        STAGE_PRINT("  Required fields: %d/10\n", found_fields);
        for (int i = 0; i < error_count; i++) {
            STAGE_PRINT("  %s\n", errors[i]);
        }
        error_reporter_set_stage_status(STAGE_SEMANTIC, STAGE_STATUS_FAIL);
        return false;
    }
}

/* Stage 5A: Pattern Matching Engine */
bool run_stage5a_pattern_matching(const JsonValue *root) {
    pattern_matcher_init();

    static const char *pattern_fields[8] = {
        FIELD_CUSTOMER_ID,
        FIELD_FULL_NAME,
        FIELD_EMAIL,
        FIELD_PHONE,
        FIELD_DOB,
        FIELD_PAN,
        FIELD_ACCOUNT_NO,
        FIELD_IFSC
    };

    PatternResult results[8];
    int line_nums[8];
    int col_nums[8];
    int failures = 0;

    for (int i = 0; i < 8; i++) {
        JsonObjectMember *mem = json_object_find_member(root, pattern_fields[i]);
        if (!mem || mem->value->type != JSON_STRING) {
            results[i].is_valid = false;
            snprintf(results[i].error_message, sizeof(results[i].error_message), "Field missing or invalid type for pattern check");
            line_nums[i] = mem ? mem->line : 0;
            col_nums[i] = mem ? mem->col : 0;
            failures++;
            continue;
        }

        const char *val = mem->value->data.string_val;
        line_nums[i] = mem->value->line ? mem->value->line : mem->line;
        col_nums[i] = mem->value->col ? mem->value->col : mem->col;

        switch (i) {
            case 0: results[i] = validate_customer_id(val); break;
            case 1: results[i] = validate_full_name(val); break;
            case 2: results[i] = validate_email(val); break;
            case 3: results[i] = validate_phone(val); break;
            case 4: results[i] = validate_date_of_birth(val); break;
            case 5: results[i] = validate_pan_number(val); break;
            case 6: results[i] = validate_account_number(val); break;
            case 7: results[i] = validate_ifsc_code(val); break;
        }

        if (!results[i].is_valid) {
            failures++;
            error_reporter_add_error(STAGE_PATTERN, "RULE_PATTERN_FORMAT", pattern_fields[i],
                                     line_nums[i], col_nums[i], results[i].error_message);
            error_reporter_set_field_status(pattern_fields[i], FIELD_STATUS_FAIL, results[i].error_message);
        } else {
            error_reporter_set_field_status(pattern_fields[i], FIELD_STATUS_PASS, NULL);
        }
    }

    if (failures == 0) {
        STAGE_PRINT("Pattern Matching Engine: PASS\n");
        for (int i = 0; i < 8; i++) {
            STAGE_PRINT("  %s: PASS\n", pattern_fields[i]);
        }
        error_reporter_set_stage_status(STAGE_PATTERN, STAGE_STATUS_PASS);
    } else {
        STAGE_PRINT("Pattern Matching Engine: FAIL\n");
        for (int i = 0; i < 8; i++) {
            if (results[i].is_valid) {
                STAGE_PRINT("  %s: PASS\n", pattern_fields[i]);
            } else {
                STAGE_PRINT("  %s: FAIL\n", pattern_fields[i]);
                STAGE_PRINT("    Pattern Matching Engine rejects it: Field '%s' (Line %d, Col %d): %s\n",
                            pattern_fields[i], line_nums[i], col_nums[i], results[i].error_message);
            }
        }
        error_reporter_set_stage_status(STAGE_PATTERN, STAGE_STATUS_FAIL);
    }

    pattern_matcher_cleanup();
    return (failures == 0);
}

/* Stage 5B: Domain Rule Engine */
bool run_stage5b_domain_rules(const JsonValue *root) {
    JsonObjectMember *acc_mem = json_object_find_member(root, FIELD_ACCOUNT_TYPE);
    JsonObjectMember *dep_mem = json_object_find_member(root, FIELD_INITIAL_DEPOSIT);

    const char *acc_type_sval = (acc_mem && acc_mem->value->type == JSON_STRING) ? acc_mem->value->data.string_val : "";
    double deposit_val = (dep_mem && dep_mem->value->type == JSON_NUMBER) ? dep_mem->value->data.number_val.num_val : -1.0;

    int acc_line = acc_mem ? (acc_mem->value->line ? acc_mem->value->line : acc_mem->line) : 0;
    int acc_col  = acc_mem ? (acc_mem->value->col ? acc_mem->value->col : acc_mem->col) : 0;
    int dep_line = dep_mem ? (dep_mem->value->line ? dep_mem->value->line : dep_mem->line) : 0;
    int dep_col  = dep_mem ? (dep_mem->value->col ? dep_mem->value->col : dep_mem->col) : 0;

    DomainRuleResult dr_acc = domain_check_account_type(acc_type_sval);
    DomainRuleResult dr_nn  = domain_check_non_negative_deposit(deposit_val);
    DomainRuleResult dr_min;
    dr_min.is_valid = true;
    dr_min.error_message[0] = '\0';

    if (dr_acc.is_valid && dr_nn.is_valid) {
        dr_min = domain_check_deposit_limits(acc_type_sval, deposit_val);
    }

    bool all_ok = dr_acc.is_valid && dr_nn.is_valid && dr_min.is_valid;

    if (all_ok) {
        STAGE_PRINT("Domain Rule Engine: PASS\n");
        STAGE_PRINT("  Account type: PASS\n");
        STAGE_PRINT("  Non-negative deposit: PASS\n");
        STAGE_PRINT("  Applicable minimum deposit: PASS\n");
        error_reporter_set_field_status(FIELD_ACCOUNT_TYPE, FIELD_STATUS_PASS, NULL);
        error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_PASS, NULL);
        error_reporter_set_stage_status(STAGE_DOMAIN, STAGE_STATUS_PASS);
    } else {
        STAGE_PRINT("Domain Rule Engine: FAIL\n");
        if (dr_acc.is_valid) {
            STAGE_PRINT("  Account type: PASS\n");
        } else {
            STAGE_PRINT("  Account type: FAIL\n");
            STAGE_PRINT("    Domain Rule Engine rejects it: Field '%s': %s\n", FIELD_ACCOUNT_TYPE, dr_acc.error_message);
            error_reporter_add_error(STAGE_DOMAIN, "RULE_DOMAIN_ACCOUNT_TYPE", FIELD_ACCOUNT_TYPE,
                                     acc_line, acc_col, dr_acc.error_message);
            error_reporter_set_field_status(FIELD_ACCOUNT_TYPE, FIELD_STATUS_FAIL, dr_acc.error_message);
        }

        if (dr_nn.is_valid) {
            STAGE_PRINT("  Non-negative deposit: PASS\n");
        } else {
            STAGE_PRINT("  Non-negative deposit: FAIL\n");
            STAGE_PRINT("    Domain Rule Engine rejects it: Field '%s': %s\n", FIELD_INITIAL_DEPOSIT, dr_nn.error_message);
            error_reporter_add_error(STAGE_DOMAIN, "RULE_DOMAIN_NON_NEGATIVE", FIELD_INITIAL_DEPOSIT,
                                     dep_line, dep_col, dr_nn.error_message);
            error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_FAIL, dr_nn.error_message);
        }

        if (!dr_acc.is_valid) {
            STAGE_PRINT("  Applicable minimum deposit: NOT APPLICABLE (Invalid account type)\n");
        } else if (!dr_nn.is_valid) {
            STAGE_PRINT("  Applicable minimum deposit: FAIL (Deposit is negative)\n");
        } else if (dr_min.is_valid) {
            STAGE_PRINT("  Applicable minimum deposit: PASS\n");
            error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_PASS, NULL);
        } else {
            STAGE_PRINT("  Applicable minimum deposit: FAIL\n");
            STAGE_PRINT("    Domain Rule Engine rejects it: Field '%s': %s\n", FIELD_INITIAL_DEPOSIT, dr_min.error_message);
            error_reporter_add_error(STAGE_DOMAIN, "RULE_DOMAIN_MIN_DEPOSIT", FIELD_INITIAL_DEPOSIT,
                                     dep_line, dep_col, dr_min.error_message);
            error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_FAIL, dr_min.error_message);
        }
        error_reporter_set_stage_status(STAGE_DOMAIN, STAGE_STATUS_FAIL);
    }

    return all_ok;
}

/* Sequential pipeline wrapper */
bool run_semantic_analysis(const JsonValue *root) {
    bool s4 = run_stage4_semantic_analysis(root);
    if (!s4) {
        return false;
    }

    STAGE_PRINT("\n");
    bool s5a = run_stage5a_pattern_matching(root);

    STAGE_PRINT("\n");
    bool s5b = run_stage5b_domain_rules(root);
    return s5a && s5b;
}
