# PowerShell Test Runner for Windows Users
# Executes the test harness within WSL environment

Write-Host "Executing Compiler Design Test Suite via WSL..." -ForegroundColor Cyan
wsl make test
if ($LASTEXITCODE -eq 0) {
    Write-Host "`nAll tests completed successfully!" -ForegroundColor Green
} else {
    Write-Host "`nSome tests failed." -ForegroundColor Red
}
exit $LASTEXITCODE
