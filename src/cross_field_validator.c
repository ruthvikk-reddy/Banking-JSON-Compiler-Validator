#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>
#include "cross_field_validator.h"
#include "error_reporter.h"

extern bool g_report_json_mode;
#define STAGE_PRINT(...) do { if (!g_report_json_mode) printf(__VA_ARGS__); } while (0)

static const BankRoutingConfig g_known_banks[] = {
    { "SBIN", "State Bank of India",    11, 17 },
    { "HDFC", "HDFC Bank",               11, 16 },
    { "ICIC", "ICICI Bank",              11, 18 },
    { "PUNB", "Punjab National Bank",    11, 18 },
    { "BARB", "Bank of Baroda",          11, 18 },
    { "CNRB", "Canara Bank",             11, 18 }
};

static const size_t g_num_known_banks = sizeof(g_known_banks) / sizeof(g_known_banks[0]);

CrossFieldCheckResult cross_check_account_deposit(const char *account_type, double deposit, int line, int col) {
    CrossFieldCheckResult res;
    res.is_valid = true;
    res.error_message[0] = '\0';
    res.line = line;
    res.col = col;
    res.rule_id = "RULE_CROSS_ACC_DEP";
    res.field_name = FIELD_INITIAL_DEPOSIT;

    if (!account_type) return res;

    /* Negative deposits violate Stage 5B domain rules; avoid duplicate minimum-deposit errors */
    if (deposit < -1e-9) return res;

    if (strcmp(account_type, ACCOUNT_TYPE_SAVINGS) == 0) {
        if (deposit < (MIN_DEPOSIT_SAVINGS - 1e-9)) {
            res.is_valid = false;
            snprintf(res.error_message, sizeof(res.error_message),
                     "SAVINGS account requires a minimum initial deposit of %.2f (provided %.2f).",
                     MIN_DEPOSIT_SAVINGS, deposit);
        }
    } else if (strcmp(account_type, ACCOUNT_TYPE_CURRENT) == 0) {
        if (deposit < (MIN_DEPOSIT_CURRENT - 1e-9)) {
            res.is_valid = false;
            snprintf(res.error_message, sizeof(res.error_message),
                     "CURRENT account requires a minimum initial deposit of %.2f (provided %.2f).",
                     MIN_DEPOSIT_CURRENT, deposit);
        }
    }
    return res;
}

CrossFieldCheckResult cross_check_ifsc_account_routing(const char *ifsc_code, const char *account_number, int line, int col) {
    CrossFieldCheckResult res;
    res.is_valid = true;
    res.error_message[0] = '\0';
    res.line = line;
    res.col = col;
    res.rule_id = "RULE_CROSS_IFSC_ROUTING";
    res.field_name = FIELD_ACCOUNT_NO;

    if (!ifsc_code || !account_number) return res;

    /* Verify account number is not dummy repetitive digits */
    size_t acc_len = strlen(account_number);
    if (acc_len >= 9) {
        bool all_same = true;
        for (size_t i = 1; i < acc_len; i++) {
            if (account_number[i] != account_number[0]) {
                all_same = false;
                break;
            }
        }
        if (all_same) {
            res.is_valid = false;
            snprintf(res.error_message, sizeof(res.error_message),
                     "Account number '%s' is an invalid dummy sequence and violates bank routing constraints.",
                     account_number);
            return res;
        }
    }

    /* Verify IFSC prefix bank code format */
    size_t ifsc_len = strlen(ifsc_code);
    if (ifsc_len >= 4) {
        for (int i = 0; i < 4; i++) {
            if (!isupper((unsigned char)ifsc_code[i])) {
                res.is_valid = false;
                res.field_name = FIELD_IFSC;
                snprintf(res.error_message, sizeof(res.error_message),
                         "IFSC code '%s' has invalid bank identifier prefix (must be 4 uppercase letters).",
                         ifsc_code);
                return res;
            }
        }

        /* Check against known demonstrable bank routing table */
        char prefix[5];
        strncpy(prefix, ifsc_code, 4);
        prefix[4] = '\0';

        for (size_t i = 0; i < g_num_known_banks; i++) {
            if (strcmp(prefix, g_known_banks[i].prefix) == 0) {
                if ((int)acc_len < g_known_banks[i].min_acc_len || (int)acc_len > g_known_banks[i].max_acc_len) {
                    res.is_valid = false;
                    snprintf(res.error_message, sizeof(res.error_message),
                             "%s routing requires account number length between %d and %d digits (provided %zu digits).",
                             g_known_banks[i].bank_name, g_known_banks[i].min_acc_len,
                             g_known_banks[i].max_acc_len, acc_len);
                    return res;
                }
                break;
            }
        }
        /* Unknown bank prefixes are handled safely and accepted */
    }

    return res;
}

CrossFieldCheckResult cross_check_age_eligibility(const char *dob_str, int line, int col) {
    CrossFieldCheckResult res;
    res.is_valid = true;
    res.error_message[0] = '\0';
    res.line = line;
    res.col = col;
    res.rule_id = "RULE_CROSS_AGE";
    res.field_name = FIELD_DOB;

    if (!dob_str || strlen(dob_str) != 10) return res;

    int birth_y = 0, birth_m = 0, birth_d = 0;
    if (sscanf(dob_str, "%d-%d-%d", &birth_y, &birth_m, &birth_d) != 3) {
        return res;
    }

    time_t now = time(NULL);
    struct tm *tm_now = localtime(&now);
    if (!tm_now) return res;

    int cur_y = tm_now->tm_year + 1900;
    int cur_m = tm_now->tm_mon + 1;
    int cur_d = tm_now->tm_mday;

    int age = cur_y - birth_y;
    if (cur_m < birth_m || (cur_m == birth_m && cur_d < birth_d)) {
        age--;
    }

    if (age < 0) {
        /* Future date of birth is already handled by Stage 5A */
        return res;
    }

    if (age < 18) {
        res.is_valid = false;
        snprintf(res.error_message, sizeof(res.error_message),
                 "Applicant age (%d years, DOB: %s) is below the minimum legal majority age of 18 for independent account opening.",
                 age, dob_str);
    } else if (age > 120) {
        res.is_valid = false;
        snprintf(res.error_message, sizeof(res.error_message),
                 "Applicant age (%d years, DOB: %s) exceeds maximum credible customer age (120 years).",
                 age, dob_str);
    }

    return res;
}

CrossFieldReport validate_all_cross_field_rules(const JsonValue *root) {
    CrossFieldReport report;
    memset(&report, 0, sizeof(CrossFieldReport));
    report.overall_valid = true;

    if (!root || root->type != JSON_OBJECT) {
        report.overall_valid = false;
        return report;
    }

    JsonObjectMember *acc_type_mem = json_object_find_member(root, FIELD_ACCOUNT_TYPE);
    JsonObjectMember *dep_mem      = json_object_find_member(root, FIELD_INITIAL_DEPOSIT);
    JsonObjectMember *ifsc_mem     = json_object_find_member(root, FIELD_IFSC);
    JsonObjectMember *acc_no_mem   = json_object_find_member(root, FIELD_ACCOUNT_NO);
    JsonObjectMember *dob_mem      = json_object_find_member(root, FIELD_DOB);

    const char *acc_type_str = (acc_type_mem && acc_type_mem->value->type == JSON_STRING)
                               ? acc_type_mem->value->data.string_val : NULL;
    double deposit_val       = (dep_mem && dep_mem->value->type == JSON_NUMBER)
                               ? dep_mem->value->data.number_val.num_val : -1.0;
    const char *ifsc_str     = (ifsc_mem && ifsc_mem->value->type == JSON_STRING)
                               ? ifsc_mem->value->data.string_val : NULL;
    const char *acc_no_str   = (acc_no_mem && acc_no_mem->value->type == JSON_STRING)
                               ? acc_no_mem->value->data.string_val : NULL;
    const char *dob_str      = (dob_mem && dob_mem->value->type == JSON_STRING)
                               ? dob_mem->value->data.string_val : NULL;

    int dep_line = dep_mem ? dep_mem->value->line : 0;
    int dep_col  = dep_mem ? dep_mem->value->col : 0;
    int acc_line = acc_no_mem ? acc_no_mem->value->line : 0;
    int acc_col  = acc_no_mem ? acc_no_mem->value->col : 0;
    int dob_line = dob_mem ? dob_mem->value->line : 0;
    int dob_col  = dob_mem ? dob_mem->value->col : 0;

    report.account_deposit = cross_check_account_deposit(acc_type_str, deposit_val, dep_line, dep_col);
    report.ifsc_routing    = cross_check_ifsc_account_routing(ifsc_str, acc_no_str, acc_line, acc_col);
    report.age_eligibility = cross_check_age_eligibility(dob_str, dob_line, dob_col);

    if (!report.account_deposit.is_valid ||
        !report.ifsc_routing.is_valid ||
        !report.age_eligibility.is_valid) {
        report.overall_valid = false;
    }

    return report;
}

bool run_stage6_cross_field_validation(const JsonValue *root) {
    CrossFieldReport report = validate_all_cross_field_rules(root);

    if (report.overall_valid) {
        STAGE_PRINT("Cross-Field Validation: PASS\n");
        STAGE_PRINT("  Account vs deposit consistency: PASS\n");
        STAGE_PRINT("  IFSC prefix vs account routing: PASS\n");
        STAGE_PRINT("  Age eligibility (>= 18 years): PASS\n");
        error_reporter_set_stage_status(STAGE_CROSS_FIELD, STAGE_STATUS_PASS);
    } else {
        STAGE_PRINT("Cross-Field Validation: FAIL\n");
        if (report.account_deposit.is_valid) {
            STAGE_PRINT("  Account vs deposit consistency: PASS\n");
        } else {
            STAGE_PRINT("  Account vs deposit consistency: FAIL\n");
            STAGE_PRINT("    Cross-Field Validation rejects it: %s\n", report.account_deposit.error_message);
            error_reporter_add_error(STAGE_CROSS_FIELD, report.account_deposit.rule_id,
                                     report.account_deposit.field_name,
                                     report.account_deposit.line, report.account_deposit.col,
                                     report.account_deposit.error_message);
            error_reporter_set_field_status(FIELD_INITIAL_DEPOSIT, FIELD_STATUS_FAIL, report.account_deposit.error_message);
        }

        if (report.ifsc_routing.is_valid) {
            STAGE_PRINT("  IFSC prefix vs account routing: PASS\n");
        } else {
            STAGE_PRINT("  IFSC prefix vs account routing: FAIL\n");
            STAGE_PRINT("    Cross-Field Validation rejects it: %s\n", report.ifsc_routing.error_message);
            error_reporter_add_error(STAGE_CROSS_FIELD, report.ifsc_routing.rule_id,
                                     report.ifsc_routing.field_name,
                                     report.ifsc_routing.line, report.ifsc_routing.col,
                                     report.ifsc_routing.error_message);
            error_reporter_set_field_status(FIELD_ACCOUNT_NO, FIELD_STATUS_FAIL, report.ifsc_routing.error_message);
        }

        if (report.age_eligibility.is_valid) {
            STAGE_PRINT("  Age eligibility (>= 18 years): PASS\n");
        } else {
            STAGE_PRINT("  Age eligibility (>= 18 years): FAIL\n");
            STAGE_PRINT("    Cross-Field Validation rejects it: %s\n", report.age_eligibility.error_message);
            error_reporter_add_error(STAGE_CROSS_FIELD, report.age_eligibility.rule_id,
                                     report.age_eligibility.field_name,
                                     report.age_eligibility.line, report.age_eligibility.col,
                                     report.age_eligibility.error_message);
            error_reporter_set_field_status(FIELD_DOB, FIELD_STATUS_FAIL, report.age_eligibility.error_message);
        }
        error_reporter_set_stage_status(STAGE_CROSS_FIELD, STAGE_STATUS_FAIL);
    }

    return report.overall_valid;
}
