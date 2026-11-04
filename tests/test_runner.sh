#!/bin/bash
# ==============================================================================
# Automated Test Suite for Domain-Aware Banking JSON Validator (Stages 1–8)
# Team 6: Ruthvik Reddy Thummala, Abhinav MS, V Vinay Karthik
# ==============================================================================

set -e

BIN="./banking_validator"
if [ ! -x "$BIN" ]; then
    echo "Executable $BIN not found! Building now..."
    make
fi

# Ensure test cases are generated
if [ ! -d "tests/cases" ] || [ -z "$(ls -A tests/cases 2>/dev/null)" ]; then
    echo "Generating regression test cases..."
    python3 tests/generate_test_cases.py
fi

TOTAL=0
PASSED=0
FAILED=0

run_test() {
    local test_num="$1"
    local desc="$2"
    local file="$3"
    local expect_status="$4" # 0 for SUCCESS, 1 for FAILURE
    local expect_keyword="$5"

    TOTAL=$((TOTAL + 1))
    printf "Test %02d: %-54s ... " "$test_num" "$desc"

    set +e
    output=$("$BIN" $file 2>&1)
    status=$?
    set -e

    local ok=1

    # Check return code
    if [ "$expect_status" -eq 0 ] && [ "$status" -ne 0 ]; then
        ok=0
    elif [ "$expect_status" -ne 0 ] && [ "$status" -eq 0 ]; then
        ok=0
    fi

    # Check keyword if specified
    if [ -n "$expect_keyword" ]; then
        if ! echo "$output" | grep -q "$expect_keyword"; then
            ok=0
        fi
    fi

    if [ "$ok" -eq 1 ]; then
        echo -e "\033[0;32m[PASS]\033[0m"
        PASSED=$((PASSED + 1))
    else
        echo -e "\033[0;31m[FAIL]\033[0m"
        echo "  Expected Exit Status: $expect_status, Got: $status"
        if [ -n "$expect_keyword" ]; then
            echo "  Expected Keyword: \"$expect_keyword\""
        fi
        echo "  Output snippet:"
        echo "$output" | head -n 12 | sed 's/^/    /'
        FAILED=$((FAILED + 1))
    fi
}

run_json_test() {
    local test_num="$1"
    local desc="$2"
    local file="$3"
    local expect_status="$4"
    local check_python_expr="$5"

    TOTAL=$((TOTAL + 1))
    printf "Test %02d: %-54s ... " "$test_num" "$desc"

    set +e
    output=$("$BIN" --report-json "$file" 2>&1)
    status=$?
    set -e

    local ok=1

    if [ "$expect_status" -eq 0 ] && [ "$status" -ne 0 ]; then
        ok=0
    elif [ "$expect_status" -ne 0 ] && [ "$status" -eq 0 ]; then
        ok=0
    fi

    # Verify that output parses as valid JSON via python and matches expression
    if ! echo "$output" | python3 -c "import sys, json; data = json.load(sys.stdin); $check_python_expr" 2>/dev/null; then
        ok=0
    fi

    if [ "$ok" -eq 1 ]; then
        echo -e "\033[0;32m[PASS]\033[0m"
        PASSED=$((PASSED + 1))
    else
        echo -e "\033[0;31m[FAIL]\033[0m"
        echo "  Expected Exit Status: $expect_status, Got: $status"
        echo "  Output snippet:"
        echo "$output" | head -n 12 | sed 's/^/    /'
        FAILED=$((FAILED + 1))
    fi
}

echo "=============================================================================="
echo "    COMPILER DESIGN PROJECT - AUTOMATED TEST HARNESS (TEAM 6)"
echo "    Domain-Aware JSON Validation and Pattern Matching System (Stages 1–8)"
echo "=============================================================================="
echo ""
echo "--- SECTION 1: CORE GRAMMAR, PATTERNS & DOMAIN RULES ---"

run_test 1  "Valid JSON and valid banking record"            "examples/valid_customer.json"           0 "VALID BANKING DATA"
run_test 2  "Invalid JSON syntax (missing comma)"           "examples/invalid_syntax.json"          1 "SYNTAX ERROR"
run_test 3  "Invalid email pattern"                         "examples/test_invalid_email.json"       1 "Invalid email address"
run_test 4  "Invalid phone pattern (missing +91-)"          "examples/test_invalid_phone.json"       1 "Invalid phone number"
run_test 5  "Invalid PAN pattern"                           "examples/test_invalid_pan.json"         1 "Invalid PAN number"
run_test 6  "Invalid IFSC code pattern"                     "examples/test_invalid_ifsc.json"        1 "Invalid IFSC code"
run_test 7  "Invalid Customer ID pattern"                   "examples/test_invalid_cusid.json"       1 "Invalid Customer ID format"
run_test 8  "Invalid account number length"                 "examples/test_invalid_accno.json"       1 "Invalid account number"
run_test 9  "Missing mandatory field (phone)"               "examples/missing_fields.json"          1 "MISSING"
run_test 10 "Wrong JSON data type for numeric field"        "examples/test_wrong_type.json"          1 "TYPE ERROR"
run_test 11 "Invalid account type ('BUSINESS')"             "examples/test_invalid_acctype.json"     1 "Invalid account type"
run_test 12 "Negative initial deposit"                      "examples/test_negative_deposit.json"    1 "cannot be negative"
run_test 13 "SAVINGS deposit below 1000 minimum"            "examples/edge_savings_below_min.json"   1 "SAVINGS account requires a minimum"
run_test 14 "CURRENT deposit below 5000 minimum"            "examples/edge_current_below_min.json"   1 "CURRENT account requires a minimum"
run_test 15 "Future date of birth rejection"                "examples/test_future_dob.json"          1 "cannot be in the future"
run_test 16 "Invalid calendar date (Feb 29 non-leap year)"   "examples/edge_invalid_leap_date.json"   1 "not a leap year"
run_test 17 "Valid leap year date (Feb 29 leap year 2000)"  "examples/edge_valid_leap_date.json"     0 "VALID BANKING DATA"
run_test 18 "Duplicate key detection in JSON object"        "examples/edge_duplicate_key.json"      1 "Duplicate key"
run_test 19 "Unknown field warning (metadata)"              "examples/nested_json.json"             0 "Unknown field"
run_test 20 "Nested JSON objects and arrays support"        "examples/nested_json.json"             0 "VALID BANKING DATA"

echo ""
echo "--- SECTION 2: INDIVIDUAL MISSING FIELD TESTS (10 CORE FIELDS) ---"

run_test 21 "Missing field: customer_id"                    "tests/cases/missing_customer_id.json"   1 "Mandatory field 'customer_id' is missing"
run_test 22 "Missing field: full_name"                      "tests/cases/missing_full_name.json"     1 "Mandatory field 'full_name' is missing"
run_test 23 "Missing field: email"                          "tests/cases/missing_email.json"         1 "Mandatory field 'email' is missing"
run_test 24 "Missing field: phone"                          "tests/cases/missing_phone.json"         1 "Mandatory field 'phone' is missing"
run_test 25 "Missing field: date_of_birth"                  "tests/cases/missing_date_of_birth.json" 1 "Mandatory field 'date_of_birth' is missing"
run_test 26 "Missing field: pan_number"                     "tests/cases/missing_pan_number.json"    1 "Mandatory field 'pan_number' is missing"
run_test 27 "Missing field: account_number"                 "tests/cases/missing_account_number.json" 1 "Mandatory field 'account_number' is missing"
run_test 28 "Missing field: ifsc_code"                      "tests/cases/missing_ifsc_code.json"     1 "Mandatory field 'ifsc_code' is missing"
run_test 29 "Missing field: account_type"                   "tests/cases/missing_account_type.json"  1 "Mandatory field 'account_type' is missing"
run_test 30 "Missing field: initial_deposit"               "tests/cases/missing_initial_deposit.json" 1 "Mandatory field 'initial_deposit' is missing"

echo ""
echo "--- SECTION 3: EMPTY STRING (\"\") FIELD TESTS ---"

run_test 31 "Empty string: customer_id"                     "tests/cases/empty_customer_id.json"     1 "Field 'customer_id' cannot be empty"
run_test 32 "Empty string: full_name"                       "tests/cases/empty_full_name.json"       1 "Field 'full_name' cannot be empty"
run_test 33 "Empty string: email"                           "tests/cases/empty_email.json"           1 "Field 'email' cannot be empty"
run_test 34 "Empty string: phone"                           "tests/cases/empty_phone.json"           1 "Field 'phone' cannot be empty"
run_test 35 "Empty string: date_of_birth"                   "tests/cases/empty_date_of_birth.json"   1 "Field 'date_of_birth' cannot be empty"
run_test 36 "Empty string: pan_number"                      "tests/cases/empty_pan_number.json"      1 "Field 'pan_number' cannot be empty"
run_test 37 "Empty string: account_number"                  "tests/cases/empty_account_number.json"  1 "Field 'account_number' cannot be empty"
run_test 38 "Empty string: ifsc_code"                       "tests/cases/empty_ifsc_code.json"       1 "Field 'ifsc_code' cannot be empty"
run_test 39 "Empty string: account_type"                    "tests/cases/empty_account_type.json"    1 "Field 'account_type' cannot be empty"

echo ""
echo "--- SECTION 4: WHITESPACE-ONLY (\"   \") FIELD TESTS ---"

run_test 40 "Whitespace string: customer_id"                "tests/cases/whitespace_customer_id.json" 1 "cannot be blank or contain only whitespace"
run_test 41 "Whitespace string: full_name"                  "tests/cases/whitespace_full_name.json"   1 "cannot be blank or contain only whitespace"
run_test 42 "Whitespace string: email"                      "tests/cases/whitespace_email.json"       1 "cannot be blank or contain only whitespace"
run_test 43 "Whitespace string: phone"                      "tests/cases/whitespace_phone.json"       1 "cannot be blank or contain only whitespace"
run_test 44 "Whitespace string: date_of_birth"              "tests/cases/whitespace_date_of_birth.json" 1 "cannot be blank or contain only whitespace"
run_test 45 "Whitespace string: pan_number"                 "tests/cases/whitespace_pan_number.json"  1 "cannot be blank or contain only whitespace"
run_test 46 "Whitespace string: account_number"             "tests/cases/whitespace_account_number.json" 1 "cannot be blank or contain only whitespace"
run_test 47 "Whitespace string: ifsc_code"                  "tests/cases/whitespace_ifsc_code.json"   1 "cannot be blank or contain only whitespace"
run_test 48 "Whitespace string: account_type"               "tests/cases/whitespace_account_type.json" 1 "cannot be blank or contain only whitespace"

echo ""
echo "--- SECTION 5: INITIAL DEPOSIT NON-NUMERIC & BLANK TESTS ---"

run_test 49 "Initial deposit: numeric string (\"25000\")"   "tests/cases/deposit_string_number.json" 1 "Expected JSON Number"
run_test 50 "Initial deposit: empty string (\"\")"          "tests/cases/deposit_empty_string.json"  1 "found empty String"
run_test 51 "Initial deposit: whitespace-only string"       "tests/cases/deposit_whitespace_string.json" 1 "found whitespace-only String"

echo ""
echo "--- SECTION 6: MULTIPLE BLANK & EDGE CASES ---"

run_test 52 "Multiple blank/whitespace fields in record"     "tests/cases/multiple_blank_fields.json" 1 "INVALID BANKING DATA"
run_test 53 "Empty input file (0 bytes)"                     "examples/edge_empty.json"               1 "Empty input"
run_test 54 "Empty JSON object ({})"                         "examples/edge_empty_object.json"        1 "MISSING"
run_test 55 "Trailing comma syntax error"                    "examples/edge_trailing_comma.json"      1 "SYNTAX ERROR"
run_test 56 "Top-level array instead of object"              "examples/edge_top_level_array.json"     1 "Banking record must be a JSON Object"

echo ""
echo "--- SECTION 7: STAGE 5B BOUNDARIES, MULTI-PATTERN & INPUT TESTS ---"

run_test 57 "SAVINGS deposit at exact minimum (1000.00)"     "examples/edge_savings_exact_min.json"   0 "VALID BANKING DATA"
run_test 58 "CURRENT deposit at exact minimum (5000.00)"     "examples/edge_current_exact_min.json"   0 "VALID BANKING DATA"
run_test 59 "CURRENT deposit above minimum (6500.50)"        "examples/edge_current_valid.json"       0 "VALID BANKING DATA"
run_test 60 "Multiple independent pattern failures"          "examples/invalid_patterns.json"         1 "Pattern Matching Engine: FAIL"
run_test 61 "Missing input file (non-existent path)"         "examples/non_existent_file.json"        1 "Cannot open file"

echo ""
echo "--- SECTION 8: STAGE 6 CROSS-FIELD VALIDATION TESTS ---"

run_test 62 "Stage 6: Valid cross-field combination"         "tests/cases/stage6_valid_cross_field.json" 0 "Cross-Field Validation: PASS"
run_test 63 "Stage 6: Underage applicant (below 18 years)"   "tests/cases/stage6_underage.json"          1 "below the minimum legal majority age of 18"
run_test 64 "Stage 6: Age eligibility boundary (age >= 18)"  "tests/cases/stage6_exact_age_18.json"      0 "Age eligibility (>= 18 years): PASS"
run_test 65 "Stage 6: Leap-day birthday customer (Feb 29)"   "tests/cases/stage6_leap_day_birthday.json" 0 "Age eligibility (>= 18 years): PASS"
run_test 66 "Stage 6: Unknown bank prefix handled safely"    "tests/cases/stage6_unknown_bank_safe.json" 0 "Cross-Field Validation: PASS"
run_test 67 "Stage 6: Dummy sequence account number"         "tests/cases/stage6_dummy_account_sequence.json" 1 "invalid dummy sequence"
run_test 68 "Stage 6: Account/deposit inconsistency (CURRENT)" "tests/cases/stage6_account_deposit_mismatch.json" 1 "CURRENT account requires a minimum"

echo ""
echo "--- SECTION 9: STAGE 7 ERROR AGGREGATION & PIPELINE SAFETY TESTS ---"

run_test 69 "Stage 7: Multi-stage error collection buffer"   "tests/cases/stage7_multi_error.json"       1 "Diagnostic Classification Buffer"
run_test 70 "Stage 7: Syntax failure blocks AST execution"   "tests/cases/stage7_syntax_blocks_ast.json" 1 "Syntax Analyzer: FAIL"
run_test 71 "Stage 7: Missing fields safe memory handling"   "tests/cases/stage7_missing_fields_safe.json" 1 "Semantic Analyzer: FAIL"
run_test 72 "Stage 7: Clean diagnostic catalog on valid data" "examples/valid_customer.json"           0 "Diagnostic catalog: CLEAN"
run_test 73 "Stage 7: Multi-stage independent errors (3 errors)" "examples/test_three_errors.json"       1 "Invalid account type 'GOLD'"

echo ""
echo "--- SECTION 10: STAGE 8 VALIDATION REPORT & MACHINE-READABLE JSON TESTS ---"

run_test 74 "Stage 8: Complete report verified valid"        "examples/valid_customer.json"           0 "FINAL RESULT: VALID BANKING DATA"
run_test 75 "Stage 8: Warning-only maintains validity"       "tests/cases/stage8_warning_only.json"   0 "FINAL RESULT: VALID BANKING DATA"
run_json_test 76 "Stage 8: Machine-readable JSON on valid record" "examples/valid_customer.json" 0 "assert data['overall_valid'] == True and data['stages']['stage8_validation_report'] == 'PASS'"
run_json_test 77 "Stage 8: Machine-readable JSON on syntax fail" "examples/invalid_syntax.json" 1 "assert data['overall_valid'] == False and data['stages']['stage3_parser'] == 'FAIL'"
run_json_test 78 "Stage 8: Machine-readable JSON on underage" "tests/cases/stage6_underage.json" 1 "assert data['overall_valid'] == False and data['stages']['stage6_cross_field'] == 'FAIL'"
run_json_test 79 "Stage 8: Machine-readable JSON on 3 errors" "examples/test_three_errors.json" 1 "assert data['overall_valid'] == False and data['summary']['total_errors'] == 3 and data['stages']['stage5a_pattern'] == 'FAIL' and data['stages']['stage5b_domain'] == 'FAIL'"

echo ""
echo "=============================================================================="
echo "    TEST SUMMARY: $PASSED / $TOTAL Tests Passed ($FAILED Failed)"
echo "=============================================================================="

if [ "$FAILED" -eq 0 ]; then
    echo -e "\033[0;32mALL AUTOMATED TESTS PASSED SUCCESSFULLY!\033[0m"
    exit 0
else
    echo -e "\033[0;31mSOME TESTS FAILED!\033[0m"
    exit 1
fi
