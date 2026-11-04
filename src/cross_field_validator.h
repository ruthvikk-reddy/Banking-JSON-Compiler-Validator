#ifndef CROSS_FIELD_VALIDATOR_H
#define CROSS_FIELD_VALIDATOR_H

#include "../include/common.h"
#include "json_value.h"

typedef struct {
    bool is_valid;
    char error_message[256];
    int line;
    int col;
    const char *rule_id;
    const char *field_name;
} CrossFieldCheckResult;

typedef struct {
    bool overall_valid;
    CrossFieldCheckResult account_deposit;
    CrossFieldCheckResult ifsc_routing;
    CrossFieldCheckResult age_eligibility;
} CrossFieldReport;

typedef struct {
    const char *prefix;
    const char *bank_name;
    int min_acc_len;
    int max_acc_len;
} BankRoutingConfig;

/* Individual Cross-Field Checks (Stage 6) */
CrossFieldCheckResult cross_check_account_deposit(const char *account_type, double deposit, int line, int col);
CrossFieldCheckResult cross_check_ifsc_account_routing(const char *ifsc_code, const char *account_number, int line, int col);
CrossFieldCheckResult cross_check_age_eligibility(const char *dob_str, int line, int col);

/* Full Stage 6 Validation Functions */
CrossFieldReport validate_all_cross_field_rules(const JsonValue *root);
bool run_stage6_cross_field_validation(const JsonValue *root);

#endif /* CROSS_FIELD_VALIDATOR_H */
