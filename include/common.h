#ifndef COMMON_H
#define COMMON_H

#ifndef _POSIX_C_SOURCE
#define _POSIX_C_SOURCE 200809L
#endif
#ifndef _DEFAULT_SOURCE
#define _DEFAULT_SOURCE
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>

/* Project Metadata */
#define PROJECT_TITLE "Domain-Aware JSON Validation and Pattern Matching System for Banking Data"
#define PROJECT_DOMAIN "Banking Record Validation & Compiler Design"
#define TEAM_NUMBER 6

#define AUTHOR_1 "Ruthvik Reddy Thummala (24BAI0237)"
#define AUTHOR_2 "Abhinav MS (24BAI0218)"
#define AUTHOR_3 "V Vinay Karthik (24BCE0762)"

/* Core 10 Banking Fields */
#define FIELD_CUSTOMER_ID    "customer_id"
#define FIELD_FULL_NAME      "full_name"
#define FIELD_EMAIL          "email"
#define FIELD_PHONE          "phone"
#define FIELD_DOB            "date_of_birth"
#define FIELD_PAN            "pan_number"
#define FIELD_ACCOUNT_NO     "account_number"
#define FIELD_IFSC           "ifsc_code"
#define FIELD_ACCOUNT_TYPE   "account_type"
#define FIELD_INITIAL_DEPOSIT "initial_deposit"

#define NUM_CORE_FIELDS 10

/* Account Types */
#define ACCOUNT_TYPE_SAVINGS "SAVINGS"
#define ACCOUNT_TYPE_CURRENT "CURRENT"

/* Minimum Deposit Requirements */
#define MIN_DEPOSIT_SAVINGS 1000.00
#define MIN_DEPOSIT_CURRENT 5000.00

#endif /* COMMON_H */
