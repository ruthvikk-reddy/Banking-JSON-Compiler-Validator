#include "domain_rules.h"

DomainRuleResult domain_check_account_type(const char *account_type) {
    DomainRuleResult res = { .is_valid = false, .error_message = "" };
    if (!account_type) {
        snprintf(res.error_message, sizeof(res.error_message), "Null account type");
        return res;
    }

    if (strcmp(account_type, ACCOUNT_TYPE_SAVINGS) == 0 ||
        strcmp(account_type, ACCOUNT_TYPE_CURRENT) == 0) {
        res.is_valid = true;
    } else {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid account type '%s'. Allowed values are '%s' or '%s' (case-sensitive).",
                 account_type, ACCOUNT_TYPE_SAVINGS, ACCOUNT_TYPE_CURRENT);
    }
    return res;
}

DomainRuleResult domain_check_non_negative_deposit(double deposit) {
    DomainRuleResult res = { .is_valid = false, .error_message = "" };
    if (deposit < -1e-9) {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Initial deposit cannot be negative (provided %.2f).", deposit);
    } else {
        res.is_valid = true;
    }
    return res;
}

DomainRuleResult domain_check_cross_field_deposit(const char *account_type, double deposit) {
    DomainRuleResult res = { .is_valid = true, .error_message = "" };
    if (!account_type) return res;

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

DomainRuleResult domain_check_deposit_limits(const char *account_type, double deposit) {
    return domain_check_cross_field_deposit(account_type, deposit);
}

/*
 * Planned for Next Milestone: Stage 6 Cross-Field Validation Engine
 */
DomainRuleResult domain_check_pan_entity_cross_field(const char *pan, const char *customer_type) {
    (void)pan;
    (void)customer_type;
    DomainRuleResult res = {
        .is_valid = true,
        .error_message = "[DEFERRED TO FUTURE MILESTONE: Stage 6 Cross-Field Validation]"
    };
    return res;
}
