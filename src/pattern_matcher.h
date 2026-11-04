#ifndef PATTERN_MATCHER_H
#define PATTERN_MATCHER_H

#include "../include/common.h"

typedef struct {
    bool is_valid;
    char error_message[256];
} PatternResult;

void pattern_matcher_init(void);
void pattern_matcher_cleanup(void);

PatternResult validate_customer_id(const char *val);
PatternResult validate_full_name(const char *val);
PatternResult validate_email(const char *val);
PatternResult validate_phone(const char *val);
PatternResult validate_pan_number(const char *val);
PatternResult validate_ifsc_code(const char *val);
PatternResult validate_account_number(const char *val);
PatternResult validate_date_of_birth(const char *val);

#endif /* PATTERN_MATCHER_H */
