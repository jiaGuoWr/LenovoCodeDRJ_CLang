# Phase 7: Verify final artifacts and run quick smoke tests.

. $PSScriptRoot\helpers.ps1
Start-PhaseLog 'phase7-verify'

try {
    Use-ToolchainEnv

    $items = Get-ChildItem $DistDir -Recurse -File
    Write-Log '----- Final dist/ contents -----'
    foreach ($it in $items) {
        Write-Log ("  {0,12:N0}  {1}" -f $it.Length, $it.FullName)
    }

    $dll = Join-Path $DistBin 'LenovoTidyChecks.dll'
    $exe = Join-Path $DistBin 'lenovo-tidy-lsp.exe'
    $vsixA = Join-Path $DistDir 'lenovo-tidy-vscode.vsix'
    $vsixB = Join-Path $DistDir 'LenovoTidy.vsix'
    foreach ($p in @($dll, $exe, $vsixA, $vsixB)) {
        if (-not (Test-Path $p)) { throw "MISSING: $p" }
    }

    Write-Log '----- Smoke test 1: clang-tidy --list-checks with our plugin -----'
    $ct = Join-Path $DevTools 'llvm18\bin\clang-tidy.exe'
    & $ct "-load=$dll" "--checks=lenovo-*" --list-checks 2>&1 |
        Select-Object -First 30 | ForEach-Object { Write-Log $_ }
    if ($LASTEXITCODE -ne 0) {
        Write-Log "clang-tidy exit code: $LASTEXITCODE (plugin may not have loaded)" 'WARN'
    }

    Write-Log '----- Smoke test 2: lenovo-tidy-lsp.exe --version -----'
    & $exe --version 2>&1 | ForEach-Object { Write-Log $_ }
    if ($LASTEXITCODE -ne 0) {
        Write-Log "lenovo-tidy-lsp exit code: $LASTEXITCODE" 'WARN'
    }

    Write-Log '----- DONE -----'
    Write-Log "Install in VS Code : code --install-extension $vsixA"
    Write-Log "Install in VS 2022 : double-click $vsixB (then restart VS)"

    Stop-PhaseLog 'phase7-verify' 0
    exit 0
} catch {
    Write-Log "FATAL: $($_.Exception.Message)" 'ERROR'
    Write-Log $_.ScriptStackTrace 'ERROR'
    Stop-PhaseLog 'phase7-verify' 1
    exit 1
}
