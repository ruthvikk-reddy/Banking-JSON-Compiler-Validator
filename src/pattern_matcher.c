#include "pattern_matcher.h"
#include <regex.h>

static regex_t reg_customer_id;
static regex_t reg_full_name;
static regex_t reg_email;
static regex_t reg_phone;
static regex_t reg_pan;
static regex_t reg_ifsc;
static regex_t reg_account_no;
static regex_t reg_date_fmt;
static bool s_regex_initialized = false;

void pattern_matcher_init(void) {
    if (s_regex_initialized) return;

    regcomp(&reg_customer_id, "^CUS[0-9]{4}$", REG_EXTENDED);
    regcomp(&reg_full_name, "^[A-Za-z ]+$", REG_EXTENDED);
    regcomp(&reg_email, "^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$", REG_EXTENDED);
    regcomp(&reg_phone, "^\\+91-[0-9]{10}$", REG_EXTENDED);
    regcomp(&reg_pan, "^[A-Z]{5}[0-9]{4}[A-Z]$", REG_EXTENDED);
    regcomp(&reg_ifsc, "^[A-Z]{4}0[A-Z0-9]{6}$", REG_EXTENDED);
    regcomp(&reg_account_no, "^[0-9]{12}$", REG_EXTENDED);
    regcomp(&reg_date_fmt, "^[0-9]{4}-[0-9]{2}-[0-9]{2}$", REG_EXTENDED);

    s_regex_initialized = true;
}

void pattern_matcher_cleanup(void) {
    if (!s_regex_initialized) return;

    regfree(&reg_customer_id);
    regfree(&reg_full_name);
    regfree(&reg_email);
    regfree(&reg_phone);
    regfree(&reg_pan);
    regfree(&reg_ifsc);
    regfree(&reg_account_no);
    regfree(&reg_date_fmt);

    s_regex_initialized = false;
}

PatternResult validate_customer_id(const char *val) {
    PatternResult res = { .is_valid = false, .error_message = "" };
    if (!val) {
        snprintf(res.error_message, sizeof(res.error_message), "Null customer ID");
        return res;
    }
    if (regexec(&reg_customer_id, val, 0, NULL, 0) == 0) {
        res.is_valid = true;
    } else {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid Customer ID format '%s'. Must match '^CUS[0-9]{4}$' (e.g. 'CUS1025').", val);
    }
    return res;
}

PatternResult validate_full_name(const char *val) {
    PatternResult res = { .is_valid = false, .error_message = "" };
    if (!val) {
        snprintf(res.error_message, sizeof(res.error_message), "Null full name");
        return res;
    }

    /* Check not empty and not only spaces */
    bool has_alpha = false;
    for (int i = 0; val[i] != '\0'; i++) {
        if (isalpha((unsigned char)val[i])) {
            has_alpha = true;
            break;
        }
    }
    if (!has_alpha) {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Full name cannot be empty or contain only spaces/symbols.");
        return res;
    }

    if (regexec(&reg_full_name, val, 0, NULL, 0) == 0) {
        res.is_valid = true;
    } else {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid Full Name '%s'. Must contain only alphabetic characters and spaces (no digits or punctuation).", val);
    }
    return res;
}

PatternResult validate_email(const char *val) {
    PatternResult res = { .is_valid = false, .error_message = "" };
    if (!val) {
        snprintf(res.error_message, sizeof(res.error_message), "Null email address");
        return res;
    }
    if (regexec(&reg_email, val, 0, NULL, 0) == 0) {
        res.is_valid = true;
    } else {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid email address '%s'. Expected format 'username@domain.tld' (e.g. 'rahul@gmail.com').", val);
    }
    return res;
}

PatternResult validate_phone(const char *val) {
    PatternResult res = { .is_valid = false, .error_message = "" };
    if (!val) {
        snprintf(res.error_message, sizeof(res.error_message), "Null phone number");
        return res;
    }
    if (regexec(&reg_phone, val, 0, NULL, 0) == 0) {
        res.is_valid = true;
    } else {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid phone number '%s'. Must follow format '+91-XXXXXXXXXX' with exactly 10 digits.", val);
    }
    return res;
}

PatternResult validate_pan_number(const char *val) {
    PatternResult res = { .is_valid = false, .error_message = "" };
    if (!val) {
        snprintf(res.error_message, sizeof(res.error_message), "Null PAN number");
        return res;
    }
    if (regexec(&reg_pan, val, 0, NULL, 0) == 0) {
        res.is_valid = true;
    } else {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid PAN number '%s'. Must consist of 5 uppercase letters, 4 digits, and 1 uppercase letter (e.g. 'ABCDE1234F').", val);
    }
    return res;
}

PatternResult validate_ifsc_code(const char *val) {
    PatternResult res = { .is_valid = false, .error_message = "" };
    if (!val) {
        snprintf(res.error_message, sizeof(res.error_message), "Null IFSC code");
        return res;
    }
    if (regexec(&reg_ifsc, val, 0, NULL, 0) == 0) {
        res.is_valid = true;
    } else {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid IFSC code '%s'. Must be 4 uppercase letters, '0', followed by 6 alphanumeric characters (e.g. 'SBIN0001234').", val);
    }
    return res;
}

PatternResult validate_account_number(const char *val) {
    PatternResult res = { .is_valid = false, .error_message = "" };
    if (!val) {
        snprintf(res.error_message, sizeof(res.error_message), "Null account number");
        return res;
    }
    if (regexec(&reg_account_no, val, 0, NULL, 0) == 0) {
        res.is_valid = true;
    } else {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid account number '%s'. Must be a string consisting of exactly 12 numeric digits.", val);
    }
    return res;
}

PatternResult validate_date_of_birth(const char *val) {
    PatternResult res = { .is_valid = false, .error_message = "" };
    if (!val) {
        snprintf(res.error_message, sizeof(res.error_message), "Null date of birth");
        return res;
    }

    /* 1. Format check YYYY-MM-DD */
    if (regexec(&reg_date_fmt, val, 0, NULL, 0) != 0) {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid date format '%s'. Must be in 'YYYY-MM-DD' format.", val);
        return res;
    }

    int year = 0, month = 0, day = 0;
    if (sscanf(val, "%d-%d-%d", &year, &month, &day) != 3) {
        snprintf(res.error_message, sizeof(res.error_message), "Could not parse date components in '%s'.", val);
        return res;
    }

    /* 2. Month range check */
    if (month < 1 || month > 12) {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Invalid month '%02d' in date '%s'. Month must be between 01 and 12.", month, val);
        return res;
    }

    /* 3. Calendar day check with Leap Year handling */
    bool is_leap = ((year % 4 == 0 && year % 100 != 0) || (year % 400 == 0));
    int days_in_month[] = { 0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };
    if (is_leap) {
        days_in_month[2] = 29;
    }

    int max_day = days_in_month[month];
    if (day < 1 || day > max_day) {
        if (month == 2 && day == 29 && !is_leap) {
            snprintf(res.error_message, sizeof(res.error_message),
                     "Invalid calendar date '%s': Year %d is not a leap year, so February has only 28 days.", val, year);
        } else {
            snprintf(res.error_message, sizeof(res.error_message),
                     "Invalid day '%02d' for month %02d in date '%s'. Month has %d days.", day, month, val, max_day);
        }
        return res;
    }

    /* 4. Future date check against current system time */
    time_t now = time(NULL);
    struct tm *now_tm = localtime(&now);
    int cur_year = now_tm->tm_year + 1900;
    int cur_mon = now_tm->tm_mon + 1;
    int cur_day = now_tm->tm_mday;

    bool is_future = false;
    if (year > cur_year) {
        is_future = true;
    } else if (year == cur_year) {
        if (month > cur_mon) {
            is_future = true;
        } else if (month == cur_mon && day > cur_day) {
            is_future = true;
        }
    }

    if (is_future) {
        snprintf(res.error_message, sizeof(res.error_message),
                 "Date of birth '%s' cannot be in the future (current system date is %04d-%02d-%02d).",
                 val, cur_year, cur_mon, cur_day);
        return res;
    }

    res.is_valid = true;
    return res;
}
