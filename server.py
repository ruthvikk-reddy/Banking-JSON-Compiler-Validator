#!/usr/bin/env python3
"""
Domain-Aware JSON Validation System - Local Compiler HTTP Server
Compiler Design Project - Team 6

Invokes the compiled C/Flex/Bison 'banking_validator' executable via safe subprocess
pipes and serves the web frontend. Uses only the Python standard library.
"""

import sys
import os
import json
import subprocess
import shutil
import platform
import argparse
from http.server import HTTPServer, BaseHTTPRequestHandler
from urllib.parse import urlparse, parse_qs

# Base directories
BASE_DIR = os.path.dirname(os.path.abspath(__file__))
WEB_DIR = os.path.join(BASE_DIR, "web")
EXAMPLES_DIR = os.path.join(BASE_DIR, "examples")

# Find compiler command
def get_compiler_cmd():
    """Determine the command list to execute banking_validator safely."""
    # Check if we are running in Linux/WSL/Docker directly
    if platform.system() != "Windows":
        local_bin = os.path.join(BASE_DIR, "banking_validator")
        if not (os.path.exists(local_bin) and os.access(local_bin, os.X_OK)):
            # Attempt auto-compilation if binary is not present
            if shutil.which("make"):
                sys.stderr.write("[INFO] banking_validator binary not found. Compiling via 'make'...\n")
                try:
                    subprocess.run(["make"], cwd=BASE_DIR, check=True)
                except Exception as e:
                    sys.stderr.write(f"[WARN] Automatic compilation failed: {e}\n")
        if os.path.exists(local_bin) and os.access(local_bin, os.X_OK):
            return [local_bin, "--report-json", "-"]
        if shutil.which("banking_validator"):
            return ["banking_validator", "--report-json", "-"]
        return [local_bin, "--report-json", "-"]

    # We are on Windows
    win_exe = os.path.join(BASE_DIR, "banking_validator.exe")
    if os.path.exists(win_exe):
        return [win_exe, "--report-json", "-"]

    # Fall back to invoking via WSL
    if shutil.which("wsl"):
        local_bin = os.path.join(BASE_DIR, "banking_validator")
        if not os.path.exists(local_bin):
            sys.stderr.write("[INFO] banking_validator binary not found. Compiling via 'wsl make'...\n")
            try:
                subprocess.run(["wsl", "make"], cwd=BASE_DIR, check=True)
            except Exception as e:
                sys.stderr.write(f"[WARN] Automatic WSL compilation failed: {e}\n")
        return ["wsl", "./banking_validator", "--report-json", "-"]

    return ["./banking_validator", "--report-json", "-"]


def run_compiler(json_payload_str, timeout_sec=10):
    """
    Execute banking_validator safely via subprocess stdin/stdout.
    Returns (status_dict, return_code, error_str).
    """
    cmd = get_compiler_cmd()
    try:
        process = subprocess.run(
            cmd,
            input=json_payload_str,
            capture_output=True,
            text=True,
            cwd=BASE_DIR,
            timeout=timeout_sec
        )
        stdout = process.stdout.strip()
        stderr = process.stderr.strip()

        # The compiler returns JSON output on stdout in both success and validation failure modes
        if stdout:
            try:
                data = json.loads(stdout)
                return data, process.returncode, None
            except json.JSONDecodeError as jde:
                # If stdout wasn't JSON, return raw diagnostic
                return {
                    "overall_valid": False,
                    "final_result": "INTERNAL COMPILER OUTPUT ERROR",
                    "error": f"Failed to parse compiler JSON output: {jde}",
                    "raw_stdout": stdout,
                    "raw_stderr": stderr
                }, process.returncode, str(jde)
        else:
            return {
                "overall_valid": False,
                "final_result": "EXECUTION FAILED",
                "error": "No output produced by compiler executable.",
                "raw_stderr": stderr
            }, process.returncode, "Empty compiler output"

    except subprocess.TimeoutExpired:
        return {
            "overall_valid": False,
            "final_result": "TIMEOUT ERROR",
            "error": f"Compiler execution exceeded time limit ({timeout_sec}s)."
        }, -1, "Timeout"
    except Exception as exc:
        return {
            "overall_valid": False,
            "final_result": "SUBPROCESS ERROR",
            "error": f"Failed to invoke compiler: {exc}"
        }, -1, str(exc)


# Preset samples for demonstration and testing
PRESET_SAMPLES = {
    "valid": {
        "customer_id": "CUS1025",
        "full_name": "Rahul Sharma",
        "email": "rahul.sharma@example.com",
        "phone": "+91-9876543210",
        "date_of_birth": "2001-05-14",
        "pan_number": "ABCDE1234F",
        "account_number": "123456789012",
        "ifsc_code": "SBIN0001234",
        "account_type": "SAVINGS",
        "initial_deposit": 5000.00
    },
    "invalid_multi": {
        "customer_id": "CUS1025",
        "full_name": "Rahul Sharma",
        "email": "rahul.gmail.com",
        "phone": "+91-9876543210",
        "date_of_birth": "2001-05-14",
        "pan_number": "ABCDE1234F",
        "account_number": "123456789012",
        "ifsc_code": "SBIN0001234",
        "account_type": "GOLD",
        "initial_deposit": -500.00
    },
    "invalid_email": {
        "customer_id": "CUS1025",
        "full_name": "Rahul Sharma",
        "email": "invalid_email_format",
        "phone": "+91-9876543210",
        "date_of_birth": "2001-05-14",
        "pan_number": "ABCDE1234F",
        "account_number": "123456789012",
        "ifsc_code": "SBIN0001234",
        "account_type": "SAVINGS",
        "initial_deposit": 5000.00
    },
    "invalid_acctype": {
        "customer_id": "CUS1025",
        "full_name": "Rahul Sharma",
        "email": "rahul.sharma@example.com",
        "phone": "+91-9876543210",
        "date_of_birth": "2001-05-14",
        "pan_number": "ABCDE1234F",
        "account_number": "123456789012",
        "ifsc_code": "SBIN0001234",
        "account_type": "BUSINESS",
        "initial_deposit": 5000.00
    },
    "negative_deposit": {
        "customer_id": "CUS1025",
        "full_name": "Rahul Sharma",
        "email": "rahul.sharma@example.com",
        "phone": "+91-9876543210",
        "date_of_birth": "2001-05-14",
        "pan_number": "ABCDE1234F",
        "account_number": "123456789012",
        "ifsc_code": "SBIN0001234",
        "account_type": "SAVINGS",
        "initial_deposit": -100.00
    },
    "underage": {
        "customer_id": "CUS1025",
        "full_name": "Aarav Gupta",
        "email": "aarav.gupta@example.com",
        "phone": "+91-9876543210",
        "date_of_birth": "2015-06-15",
        "pan_number": "ABCDE1234F",
        "account_number": "123456789012",
        "ifsc_code": "SBIN0001234",
        "account_type": "SAVINGS",
        "initial_deposit": 2500.00
    },
    "syntax_error": "{\n  \"customer_id\": \"CUS1025\",\n  \"email\": \"rahul@example.com\"\n  \"phone\": \"+91-9876543210\"\n}"
}


class BankingHTTPRequestHandler(BaseHTTPRequestHandler):
    """HTTP Request Handler for Compiler Web Interface."""

    server_version = "BankingValidatorHTTP/1.0"

    def log_message(self, format, *args):
        # Clean terminal logging
        sys.stderr.write(f"[{self.log_date_time_string()}] {format % args}\n")

    def send_json_response(self, status_code, data):
        """Send JSON response with UTF-8 encoding."""
        body = json.dumps(data, indent=2).encode("utf-8")
        self.send_response(status_code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(body)))
        self.send_header("Access-Control-Allow-Origin", "*")
        self.end_headers()
        self.wfile.write(body)

    def send_static_file(self, rel_path, content_type):
        """Serve a static file safely from the web/ directory."""
        abs_path = os.path.abspath(os.path.join(WEB_DIR, rel_path))
        # Prevent directory traversal outside WEB_DIR
        try:
            common = os.path.commonpath([WEB_DIR, abs_path])
        except ValueError:
            self.send_error(403, "Forbidden")
            return

        if common != os.path.abspath(WEB_DIR) or not os.path.isfile(abs_path):
            self.send_error(404, "File Not Found")
            return

        with open(abs_path, "rb") as f:
            content = f.read()

        self.send_response(200)
        self.send_header("Content-Type", content_type)
        self.send_header("Content-Length", str(len(content)))
        self.end_headers()
        self.wfile.write(content)

    def do_OPTIONS(self):
        """Handle CORS pre-flight."""
        self.send_response(204)
        self.send_header("Access-Control-Allow-Origin", "*")
        self.send_header("Access-Control-Allow-Methods", "GET, POST, OPTIONS")
        self.send_header("Access-Control-Allow-Headers", "Content-Type")
        self.end_headers()

    def do_GET(self):
        """Handle GET requests."""
        parsed = urlparse(self.path)
        path = parsed.path

        if path in ("/", "/index.html"):
            self.send_static_file("index.html", "text/html; charset=utf-8")
        elif path == "/style.css":
            self.send_static_file("style.css", "text/css; charset=utf-8")
        elif path == "/app.js":
            self.send_static_file("app.js", "application/javascript; charset=utf-8")
        elif path.startswith("/web/"):
            rel = path[5:]
            ext = os.path.splitext(rel)[1].lower()
            mime_map = {
                ".html": "text/html; charset=utf-8",
                ".css": "text/css; charset=utf-8",
                ".js": "application/javascript; charset=utf-8",
                ".json": "application/json; charset=utf-8",
                ".svg": "image/svg+xml",
                ".png": "image/png"
            }
            mime = mime_map.get(ext, "application/octet-stream")
            self.send_static_file(rel, mime)
        elif path == "/api/health":
            cmd = get_compiler_cmd()
            self.send_json_response(200, {
                "status": "healthy",
                "compiler_command": cmd,
                "platform": platform.platform(),
                "project": "Domain-Aware JSON Validation and Pattern Matching System",
                "team": "Team 6"
            })
        elif path == "/api/samples":
            self.send_json_response(200, PRESET_SAMPLES)
        elif path.startswith("/api/samples/"):
            key = path[len("/api/samples/"):]
            if key in PRESET_SAMPLES:
                self.send_json_response(200, PRESET_SAMPLES[key])
            else:
                self.send_json_response(404, {"error": f"Unknown sample '{key}'"})
        else:
            self.send_error(404, "Not Found")

    def do_POST(self):
        """Handle POST requests (e.g. /api/validate)."""
        parsed = urlparse(self.path)
        if parsed.path != "/api/validate":
            self.send_error(404, "Endpoint not found")
            return

        # Read request body
        try:
            content_length = int(self.headers.get("Content-Length", 0))
        except (TypeError, ValueError):
            self.send_json_response(400, {"error": "Invalid Content-Length header"})
            return

        if content_length > 2 * 1024 * 1024:  # 2MB maximum
            self.send_json_response(413, {"error": "Payload exceeds 2MB limit"})
            return

        body_bytes = self.rfile.read(content_length)
        try:
            body_str = body_bytes.decode("utf-8")
        except UnicodeDecodeError:
            self.send_json_response(400, {"error": "Invalid UTF-8 payload"})
            return

        # Determine raw JSON to feed to compiler
        json_to_validate = body_str
        try:
            parsed_req = json.loads(body_str)
            # If the client sent a wrapper {"raw_json": "..."}, use the raw string
            if isinstance(parsed_req, dict) and "raw_json" in parsed_req:
                json_to_validate = str(parsed_req["raw_json"])
            elif isinstance(parsed_req, (dict, list)):
                # Client sent a structured object directly
                json_to_validate = json.dumps(parsed_req, indent=2)
        except json.JSONDecodeError:
            # If body itself is not valid JSON (e.g. intentional syntax error from raw editor),
            # pass it directly to the compiler so the compiler's lexer/parser tests it!
            json_to_validate = body_str

        # Execute compiler pipeline
        result, return_code, err = run_compiler(json_to_validate)

        # Return compiler validation report to browser
        self.send_json_response(200, result)


def run_self_test():
    """Verify compiler integration on preset cases."""
    print("==================================================")
    print("  RUNNING COMPILER WEB INTEGRATION SELF-TESTS     ")
    print("==================================================")
    cmd = get_compiler_cmd()
    print(f"Compiler Command: {' '.join(cmd)}")

    # 1. Valid customer test
    print("\n[TEST 1] Valid Customer...")
    valid_payload = json.dumps(PRESET_SAMPLES["valid"])
    res1, code1, _ = run_compiler(valid_payload)
    if res1.get("overall_valid") is True and res1.get("summary", {}).get("total_errors") == 0:
        print("  -> PASS: Valid customer accepted successfully.")
    else:
        print(f"  -> FAIL: Expected overall_valid=True, got {res1}")
        return False

    # 2. Multi-error customer test (Email + GOLD + -500)
    print("\n[TEST 2] Multi-Error Customer (email + GOLD + -500)...")
    multi_payload = json.dumps(PRESET_SAMPLES["invalid_multi"])
    res2, code2, _ = run_compiler(multi_payload)
    tot_errors = res2.get("summary", {}).get("total_errors", 0)
    if res2.get("overall_valid") is False and tot_errors == 3:
        print(f"  -> PASS: Multi-error correctly identified all {tot_errors} independent errors.")
    else:
        print(f"  -> FAIL: Expected 3 errors, got {tot_errors}. Report: {res2}")
        return False

    # 3. Syntax error test
    print("\n[TEST 3] Syntax Error (Missing comma)...")
    res3, code3, _ = run_compiler(PRESET_SAMPLES["syntax_error"])
    stage3 = res3.get("stages", {}).get("stage3_parser")
    if res3.get("overall_valid") is False and stage3 == "FAIL":
        print("  -> PASS: Lexer/Parser syntax error caught and reported safely.")
    else:
        print(f"  -> FAIL: Expected stage3_parser=FAIL, got {stage3}")
        return False

    print("\n==================================================")
    print("  ALL 3 INTEGRATION SELF-TESTS PASSED!           ")
    print("==================================================")
    return True


def main():
    default_host = os.environ.get("HOST", "0.0.0.0")
    default_port = int(os.environ.get("PORT", "8080"))

    parser = argparse.ArgumentParser(description="Banking Validator Local Web Server")
    parser.add_argument("--host", default=default_host, help=f"Host interface (default: {default_host})")
    parser.add_argument("--port", type=int, default=default_port, help=f"Port number (default: {default_port})")
    parser.add_argument("--test", action="store_true", help="Run integration self-test and exit")
    args = parser.parse_args()

    if args.test:
        success = run_self_test()
        sys.exit(0 if success else 1)

    server_address = (args.host, args.port)
    httpd = HTTPServer(server_address, BankingHTTPRequestHandler)
    print("=================================================================")
    print("  Domain-Aware JSON Validation System - Local Web Server")
    print("  Compiler Design Project - Team 6")
    print("=================================================================")
    print(f"  Compiler Command: {' '.join(get_compiler_cmd())}")
    print(f"  Listening on: http://{args.host}:{args.port}/")
    print("  Press Ctrl+C to terminate the server.\n")

    try:
        httpd.serve_forever()
    except KeyboardInterrupt:
        print("\nServer shutting down cleanly...")
        httpd.server_close()
        sys.exit(0)


if __name__ == "__main__":
    main()
