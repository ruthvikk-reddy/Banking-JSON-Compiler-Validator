#!/usr/bin/env python3
"""
Test Case Generator for Banking JSON Validator
Team 6: Ruthvik Reddy Thummala, Abhinav MS, V Vinay Karthik
Generates regression test JSON files for missing, empty, and whitespace fields.
"""

import json
import os

BASE_DIR = os.path.dirname(os.path.abspath(__file__))
CASES_DIR = os.path.join(BASE_DIR, "cases")
os.makedirs(CASES_DIR, exist_ok=True)

VALID_RECORD = {
    "customer_id": "CUS1025",
    "full_name": "Rahul Sharma",
    "email": "rahul@gmail.com",
    "phone": "+91-9876543210",
    "date_of_birth": "2001-05-14",
    "pan_number": "ABCDE1234F",
    "account_number": "123456789012",
    "ifsc_code": "SBIN0001234",
    "account_type": "SAVINGS",
    "initial_deposit": 25000
}

ALL_FIELDS = list(VALID_RECORD.keys())
STRING_FIELDS = [k for k in ALL_FIELDS if k != "initial_deposit"]

# 1. Generate Missing Field Cases (10 tests)
for field in ALL_FIELDS:
    rec = dict(VALID_RECORD)
    del rec[field]
    path = os.path.join(CASES_DIR, f"missing_{field}.json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(rec, f, indent=2)

# 2. Generate Empty String Cases (9 tests)
for field in STRING_FIELDS:
    rec = dict(VALID_RECORD)
    rec[field] = ""
    path = os.path.join(CASES_DIR, f"empty_{field}.json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(rec, f, indent=2)

# 3. Generate Whitespace-Only String Cases (9 tests)
for field in STRING_FIELDS:
    rec = dict(VALID_RECORD)
    rec[field] = "   "
    path = os.path.join(CASES_DIR, f"whitespace_{field}.json")
    with open(path, "w", encoding="utf-8") as f:
        json.dump(rec, f, indent=2)

# 4. Generate Initial Deposit String / Blank Cases (3 tests)
rec = dict(VALID_RECORD)
rec["initial_deposit"] = "25000"
with open(os.path.join(CASES_DIR, "deposit_string_number.json"), "w", encoding="utf-8") as f:
    json.dump(rec, f, indent=2)

rec = dict(VALID_RECORD)
rec["initial_deposit"] = ""
with open(os.path.join(CASES_DIR, "deposit_empty_string.json"), "w", encoding="utf-8") as f:
    json.dump(rec, f, indent=2)

rec = dict(VALID_RECORD)
rec["initial_deposit"] = "   "
with open(os.path.join(CASES_DIR, "deposit_whitespace_string.json"), "w", encoding="utf-8") as f:
    json.dump(rec, f, indent=2)

# 5. Multiple Blank Fields Case
rec = dict(VALID_RECORD)
rec["customer_id"] = ""
rec["full_name"] = "   "
rec["email"] = ""
rec["phone"] = "   "
rec["pan_number"] = ""
rec["account_type"] = "   "
rec["initial_deposit"] = ""
with open(os.path.join(CASES_DIR, "multiple_blank_fields.json"), "w", encoding="utf-8") as f:
    json.dump(rec, f, indent=2)

print(f"Successfully generated all test cases in {CASES_DIR}")
