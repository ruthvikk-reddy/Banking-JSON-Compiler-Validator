#ifndef DOMAIN_RULES_H
#define DOMAIN_RULES_H

#include "../include/common.h"
#include "json_value.h"

typedef struct {
    bool is_valid;
    char error_message[256];
} DomainRuleResult;

/*
 * STAGE 5B: DOMAIN RULE ENGINE (Current Milestone Scope)
 * - Allowed account types (SAVINGS, CURRENT)
 * - Non-negative initial deposit
 * - Minimum deposit constraints by account type
 */
DomainRuleResult domain_check_account_type(const char *account_type);
DomainRuleResult domain_check_non_negative_deposit(double deposit);
DomainRuleResult domain_check_deposit_limits(const char *account_type, double deposit);
DomainRuleResult domain_check_cross_field_deposit(const char *account_type, double deposit);

/*
 * STAGE 6: CROSS-FIELD VALIDATION (Deferred to Next Milestone)
 * - Entity-type vs PAN 4th character validation
 * - Account-holder date of birth vs account restrictions
 * - Cross-field dependency relationships
 */
DomainRuleResult domain_check_pan_entity_cross_field(const char *pan, const char *customer_type);

#endif /* DOMAIN_RULES_H */
