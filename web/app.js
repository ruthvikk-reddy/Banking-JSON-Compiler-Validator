/**
 * Domain-Aware JSON Validation System - Frontend Controller
 * Compiler Design Project
 */

document.addEventListener("DOMContentLoaded", () => {
  // 10 Core Mandatory Banking Fields
  const CORE_FIELDS = [
    "customer_id",
    "full_name",
    "email",
    "phone",
    "date_of_birth",
    "pan_number",
    "account_number",
    "ifsc_code",
    "account_type",
    "initial_deposit"
  ];

  // Pipeline stage definitions
  const STAGE_DEFS = [
    { key: "stage1_input", label: "Stage 1", title: "Banking Input Stream", engine: "POSIX File / Stdin Stream" },
    { key: "stage2_lexer", label: "Stage 2", title: "Lexical Analysis", engine: "Flex Scanner & DFA Tokenizer" },
    { key: "stage3_parser", label: "Stage 3", title: "Syntax Analysis", engine: "Bison LALR(1) Grammar & AST" },
    { key: "stage4_semantic", label: "Stage 4", title: "Semantic Analysis", engine: "Mandatory Schema & Type Verifier" },
    { key: "stage5a_pattern", label: "Stage 5A", title: "Pattern Matching", engine: "POSIX Regex Engine (6 Formats)" },
    { key: "stage5b_domain", label: "Stage 5B", title: "Domain Rule Engine", engine: "Banking Bounds & Calendar Checks" },
    { key: "stage6_cross_field", label: "Stage 6", title: "Cross-Field Validation", engine: "Age & Account-Deposit Rules" },
    { key: "stage7_error_aggregation", label: "Stage 7", title: "Error Aggregator", engine: "Multi-Stage Diagnostic Buffer" },
    { key: "stage8_validation_report", label: "Stage 8", title: "Validation Report", engine: "Deterministic Audit Decision" }
  ];

  // DOM Elements
  const bankingForm = document.getElementById("bankingForm");
  const formViewBtn = document.getElementById("formViewBtn");
  const rawJsonViewBtn = document.getElementById("rawJsonViewBtn");
  const rawJsonModeContainer = document.getElementById("rawJsonModeContainer");
  const rawJsonTextarea = document.getElementById("rawJsonTextarea");
  const submitRawJsonBtn = document.getElementById("submitRawJsonBtn");
  const syncFormFromRawBtn = document.getElementById("syncFormFromRawBtn");

  const executionTimestamp = document.getElementById("executionTimestamp");
  const resultsPlaceholder = document.getElementById("resultsPlaceholder");
  const resultsLoading = document.getElementById("resultsLoading");
  const resultsContent = document.getElementById("resultsContent");
  const staleNotice = document.getElementById("staleNotice");

  const statusBanner = document.getElementById("statusBanner");
  const statusIcon = document.getElementById("statusIcon");
  const overallStatusText = document.getElementById("overallStatusText");
  const auditDecisionBadge = document.getElementById("auditDecisionBadge");
  const totalErrorsCount = document.getElementById("totalErrorsCount");
  const totalWarningsCount = document.getElementById("totalWarningsCount");
  const stagesRatio = document.getElementById("stagesRatio");

  const stagesGrid = document.getElementById("stagesGrid");
  const diagnosticsBlock = document.getElementById("diagnosticsBlock");
  const diagnosticsCountBadge = document.getElementById("diagnosticsCountBadge");
  const diagnosticsList = document.getElementById("diagnosticsList");
  const fieldsTableBody = document.getElementById("fieldsTableBody");
  const submittedJsonOutput = document.getElementById("submittedJsonOutput");
  const rawCompilerOutput = document.getElementById("rawCompilerOutput");

  // Track state
  let hasValidatedOnce = false;

  // -------------------------------------------------------------------------
  // Stale Results Handler
  // -------------------------------------------------------------------------
  function markResultsStale() {
    if (hasValidatedOnce && !resultsContent.classList.contains("hidden")) {
      staleNotice.classList.remove("hidden");
    }
  }

  // Attach input listeners to form controls & raw textarea to detect modifications
  CORE_FIELDS.forEach(field => {
    const input = document.getElementById(field);
    if (input) {
      input.addEventListener("input", markResultsStale);
      input.addEventListener("change", markResultsStale);
    }
  });

  rawJsonTextarea.addEventListener("input", markResultsStale);

  // -------------------------------------------------------------------------
  // View Toggle (Form View vs Raw JSON View)
  // -------------------------------------------------------------------------
  function setViewMode(mode) {
    if (mode === "raw") {
      formViewBtn.classList.remove("active");
      rawJsonViewBtn.classList.add("active");
      bankingForm.classList.add("hidden");
      rawJsonModeContainer.classList.remove("hidden");
      // Populate raw textarea with current form state if empty
      if (!rawJsonTextarea.value.trim()) {
        const currentObj = getFormDataObject();
        rawJsonTextarea.value = JSON.stringify(currentObj, null, 2);
      }
    } else {
      rawJsonViewBtn.classList.remove("active");
      formViewBtn.classList.add("active");
      rawJsonModeContainer.classList.add("hidden");
      bankingForm.classList.remove("hidden");
    }
  }

  formViewBtn.addEventListener("click", () => setViewMode("form"));
  rawJsonViewBtn.addEventListener("click", () => setViewMode("raw"));

  syncFormFromRawBtn.addEventListener("click", () => {
    try {
      const parsed = JSON.parse(rawJsonTextarea.value);
      populateForm(parsed);
      markResultsStale();
      setViewMode("form");
    } catch (err) {
      alert("Cannot sync: Raw JSON contains syntax errors:\n" + err.message);
    }
  });

  // -------------------------------------------------------------------------
  // Helper: Get Form Data Object
  // -------------------------------------------------------------------------
  function getFormDataObject() {
    const obj = {};
    CORE_FIELDS.forEach(field => {
      const input = document.getElementById(field);
      if (!input) return;
      const val = input.value.trim();
      if (field === "initial_deposit") {
        if (val === "") {
          obj[field] = "";
        } else {
          const num = parseFloat(val);
          obj[field] = isNaN(num) ? val : num;
        }
      } else {
        obj[field] = val;
      }
    });
    return obj;
  }

  // -------------------------------------------------------------------------
  // Helper: Populate Form
  // -------------------------------------------------------------------------
  function populateForm(data) {
    if (typeof data !== "object" || data === null) return;
    CORE_FIELDS.forEach(field => {
      const input = document.getElementById(field);
      if (input) {
        input.value = (data[field] !== undefined && data[field] !== null) ? data[field] : "";
        input.classList.remove("field-has-error", "field-has-success");
      }
    });
  }

  // -------------------------------------------------------------------------
  // Trigger Validation (Common Dispatcher)
  // -------------------------------------------------------------------------
  bankingForm.addEventListener("submit", (e) => {
    e.preventDefault();
    triggerValidation();
  });

  submitRawJsonBtn.addEventListener("click", () => {
    triggerValidation();
  });

  async function triggerValidation() {
    let payload;
    const isRawActive = rawJsonViewBtn.classList.contains("active");

    if (isRawActive) {
      payload = rawJsonTextarea.value;
    } else {
      payload = JSON.stringify(getFormDataObject(), null, 2);
    }

    // UI state: loading
    resultsPlaceholder.classList.add("hidden");
    resultsContent.classList.add("hidden");
    resultsLoading.classList.remove("hidden");
    staleNotice.classList.add("hidden");
    executionTimestamp.textContent = "Processing...";

    const startTime = performance.now();

    try {
      const resp = await fetch("/api/validate", {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        body: JSON.stringify({ raw_json: payload })
      });

      const data = await resp.json();
      const elapsed = (performance.now() - startTime).toFixed(1);
      executionTimestamp.textContent = `Completed in ${elapsed} ms`;

      hasValidatedOnce = true;
      renderResults(data, payload);
    } catch (err) {
      resultsLoading.classList.add("hidden");
      resultsContent.classList.remove("hidden");
      executionTimestamp.textContent = "Error";
      alert("Server communication error: " + err.message);
    }
  }

  // -------------------------------------------------------------------------
  // Render Validation Results
  // -------------------------------------------------------------------------
  function renderResults(data, submittedPayload) {
    resultsLoading.classList.add("hidden");
    resultsContent.classList.remove("hidden");
    staleNotice.classList.add("hidden");

    // 1. Overall Status Banner
    const isOverallValid = !!data.overall_valid;
    statusBanner.className = isOverallValid ? "status-banner banner-valid" : "status-banner banner-invalid";
    statusIcon.textContent = isOverallValid ? "✓" : "✕";
    overallStatusText.textContent = data.final_result || (isOverallValid ? "VALID BANKING DATA" : "INVALID BANKING DATA");
    auditDecisionBadge.textContent = data.audit_decision || (isOverallValid ? "APPROVED FOR INGESTION" : "REJECTED (NON-COMPLIANT DATA)");


    const summary = data.summary || {};
    const totalErrors = summary.total_errors !== undefined ? summary.total_errors : (isOverallValid ? 0 : 1);
    const totalWarnings = summary.total_warnings || 0;
    const stagesPassed = summary.stages_passed || (isOverallValid ? 9 : 0);

    totalErrorsCount.textContent = totalErrors;
    totalWarningsCount.textContent = totalWarnings;
    stagesRatio.textContent = `${stagesPassed} / 9`;

    const statErrorsPill = document.querySelector(".stat-errors");
    if (statErrorsPill) {
      if (totalErrors === 0) {
        statErrorsPill.classList.add("clean");
      } else {
        statErrorsPill.classList.remove("clean");
      }
    }

    // 2. Render Pipeline Stages Breakdown (Stages 1–8 with 5A and 5B)
    renderStagesGrid(data.stages || {});

    // 3. Render Diagnostics / Errors List
    renderDiagnostics(data.diagnostics || []);

    // 4. Render Mandatory Fields Validation Matrix
    renderFieldsTable(data.fields || {}, submittedPayload);

    // 5. Render Submitted JSON Panel (Readable formatted JSON or raw text)
    try {
      const parsedObj = JSON.parse(submittedPayload);
      submittedJsonOutput.textContent = JSON.stringify(parsedObj, null, 2);
    } catch (e) {
      submittedJsonOutput.textContent = submittedPayload;
    }

    // 6. Render Raw Machine-Readable Compiler Response (--report-json)
    const cleanOutput = { ...data };
    delete cleanOutput.project;
    delete cleanOutput.team_number;
    delete cleanOutput.team_members;
    rawCompilerOutput.textContent = JSON.stringify(cleanOutput, null, 2);
  }

  // -------------------------------------------------------------------------
  // Render Stages Grid
  // -------------------------------------------------------------------------
  function renderStagesGrid(stages) {
    stagesGrid.innerHTML = "";

    STAGE_DEFS.forEach(stage => {
      const status = (stages[stage.key] || "NOT EXECUTED").toUpperCase();
      let statusClass = "status-unexecuted";
      let statusIcon = "•";

      if (status === "PASS") {
        statusClass = "status-pass";
        statusIcon = "✓";
      } else if (status === "FAIL") {
        statusClass = "status-fail";
        statusIcon = "✕";
      }

      const card = document.createElement("div");
      card.className = "stage-card";
      card.innerHTML = `
        <div class="stage-header-row">
          <span class="stage-num">${stage.label}</span>
          <span class="status-pill ${statusClass}">${statusIcon} ${status}</span>
        </div>
        <div class="stage-name">${stage.title}</div>
        <div class="stage-engine">${stage.engine}</div>
      `;
      stagesGrid.appendChild(card);
    });
  }

  // -------------------------------------------------------------------------
  // Render Diagnostics List
  // -------------------------------------------------------------------------
  function renderDiagnostics(diagnostics) {
    diagnosticsList.innerHTML = "";
    const count = diagnostics.length;
    diagnosticsCountBadge.textContent = `${count} ${count === 1 ? 'Error' : 'Errors'}`;

    if (count === 0) {
      diagnosticsBlock.classList.add("hidden");
      return;
    }

    diagnosticsBlock.classList.remove("hidden");

    diagnostics.forEach(diag => {
      const item = document.createElement("div");
      const isWarn = diag.severity === "WARNING";
      item.className = isWarn ? "diag-item diag-warning" : "diag-item";

      const stageLabel = formatStageLabel(diag.stage);
      const locationStr = (diag.line !== undefined && diag.line !== null)
        ? `Line ${diag.line}, Col ${diag.col || 1}`
        : "";

      item.innerHTML = `
        <div class="diag-header">
          <span class="diag-severity">${diag.severity || 'ERROR'}</span>
          <span class="diag-stage-badge">${stageLabel}</span>
          <span class="diag-rule-id">${diag.rule_id || 'RULE_VALIDATION'}</span>
          ${diag.field ? `<span class="text-secondary text-sm">[Field: <strong>${escapeHtml(diag.field)}</strong>]</span>` : ''}
          ${locationStr ? `<span class="diag-location">${locationStr}</span>` : ''}
        </div>
        <div class="diag-message">${escapeHtml(diag.message || 'Validation error')}</div>
      `;
      diagnosticsList.appendChild(item);
    });
  }

  function formatStageLabel(stageKey) {
    const map = {
      stage1_input: "Stage 1 Input",
      stage2_lexer: "Stage 2 Lexer",
      stage3_parser: "Stage 3 Parser",
      stage4_semantic: "Stage 4 Semantic",
      stage5a_pattern: "Stage 5A Pattern",
      stage5b_domain: "Stage 5B Domain",
      stage6_cross_field: "Stage 6 Cross-Field",
      stage7_error_aggregation: "Stage 7 Error Aggregator",
      stage8_validation_report: "Stage 8 Report"
    };
    return map[stageKey] || stageKey || "Pipeline";
  }

  // -------------------------------------------------------------------------
  // Render Fields Table & Highlight Inputs
  // -------------------------------------------------------------------------
  function renderFieldsTable(fieldsData, submittedPayload) {
    fieldsTableBody.innerHTML = "";

    // Parse submitted payload if possible to display values
    let parsedSubmitted = {};
    try {
      parsedSubmitted = JSON.parse(submittedPayload);
    } catch (e) {}

    CORE_FIELDS.forEach(field => {
      const fieldInfo = fieldsData[field] || { status: "NOT CHECKED", failure_reason: null };
      const status = (fieldInfo.status || "NOT CHECKED").toUpperCase();
      const val = (parsedSubmitted && parsedSubmitted[field] !== undefined) ? String(parsedSubmitted[field]) : "—";
      const reason = fieldInfo.failure_reason;

      let pillClass = "status-unexecuted";
      let pillIcon = "•";
      if (status === "PASS") {
        pillClass = "status-pass";
        pillIcon = "✓";
      } else if (status === "FAIL") {
        pillClass = "status-fail";
        pillIcon = "✕";
      }

      const row = document.createElement("tr");
      row.innerHTML = `
        <td class="field-name-cell">${field}</td>
        <td class="field-value-cell" title="${escapeHtml(val)}">${escapeHtml(val)}</td>
        <td><span class="status-pill ${pillClass}">${pillIcon} ${status}</span></td>
        <td class="field-reason-cell ${status === 'FAIL' ? 'has-error' : ''}">
          ${status === 'FAIL' ? escapeHtml(reason || 'Validation failure') : '<span class="text-muted">Compliant with domain rule</span>'}
        </td>
      `;
      fieldsTableBody.appendChild(row);

      // Highlight input field in Form view
      const inputEl = document.getElementById(field);
      if (inputEl) {
        inputEl.classList.remove("field-has-error", "field-has-success");
        if (status === "FAIL") {
          inputEl.classList.add("field-has-error");
        } else if (status === "PASS") {
          inputEl.classList.add("field-has-success");
        }
      }
    });
  }

  function escapeHtml(str) {
    if (!str) return "";
    return String(str)
      .replace(/&/g, "&amp;")
      .replace(/</g, "&lt;")
      .replace(/>/g, "&gt;")
      .replace(/"/g, "&quot;")
      .replace(/'/g, "&#039;");
  }
});
