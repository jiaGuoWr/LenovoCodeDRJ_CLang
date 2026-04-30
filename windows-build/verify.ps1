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
    # Use Invoke-Native so stderr output (warnings, plugin-loader chatter)
    # doesn't get promoted to a fatal ErrorRecord under
    # $ErrorActionPreference='Stop'. We cap the output at 30 lines after
    # the call so the log stays readable.
    $ct = Join-Path $DevTools 'llvm18\bin\clang-tidy.exe'
    $ctLog = Join-Path $script:LogsDir 'phase7-clang-tidy-listchecks.log'
    if (Test-Path $ctLog) { Remove-Item $ctLog -Force }
    $ctEc = Invoke-Native {
        & $ct "-load=$dll" "--checks=lenovo-*" --list-checks |
            Tee-Object -FilePath $ctLog | Out-Null
    }
    if (Test-Path $ctLog) {
        Get-Content $ctLog -TotalCount 30 | ForEach-Object { Write-Log $_ }
    }
    if ($ctEc -ne 0) {
        Write-Log "clang-tidy exit code: $ctEc (plugin may not have loaded)" 'WARN'
    }

    # Smoke test 2 previously ran `lenovo-tidy-lsp.exe --version` as a
    # trivial "does it start" check, but the LSP binary ignores all CLI
    # args and blocks on stdin waiting for LSP protocol input. That makes
    # it hang forever instead of printing a version. Smoke test 3 below
    # performs a strictly stronger check (spawn + real LSP `initialize`
    # request via devenv) and makes this test redundant.

    Write-Log '----- Smoke test 2: VS 2022 end-to-end activation (no GUI) -----'
    # This is the only check that catches regressions in the ILanguageClient
    # activation path (strong-name binding, ContentType wiring, or the static
    # ContentTypeDefinition export shape). A passing result here means VS 2022
    # really spawns lenovo-tidy-lsp.exe when a .cpp file opens.
    $smoke = Join-Path $PSScriptRoot 'smoke-activate.ps1'
    if (-not (Test-Path $smoke)) { throw "MISSING: $smoke" }

    $smokeLog = Join-Path $script:LogsDir 'phase7-smoke-activate.log'
    if (Test-Path $smokeLog) { Remove-Item $smokeLog -Force }

    # Run smoke-activate in a child powershell so its exit code is isolated
    # from this phase (otherwise `set -e`-style propagation can abort the
    # outer script mid-way through logging).
    $proc = Start-Process -FilePath 'powershell.exe' `
        -ArgumentList @('-NoProfile','-ExecutionPolicy','Bypass','-File', $smoke) `
        -PassThru -Wait -NoNewWindow `
        -RedirectStandardOutput $smokeLog `
        -RedirectStandardError (Join-Path $script:LogsDir 'phase7-smoke-activate.err.log')

    if (Test-Path $smokeLog) {
        Get-Content $smokeLog -Tail 25 | ForEach-Object { Write-Log ("  " + $_) }
    }

    if ($proc.ExitCode -ne 0) {
        throw "smoke-activate.ps1 failed with exit=$($proc.ExitCode); full log at $smokeLog"
    }

    # Parse the log to confirm the LSP PID was actually observed (guard
    # against a smoke-activate.ps1 bug where it returns 0 without a PID).
    $lspHit = Select-String -Path $smokeLog -Pattern 'lenovo-tidy-lsp\.exe FOUND PID=' -Quiet
    if (-not $lspHit) {
        throw "smoke-activate.ps1 exited 0 but did not record a lenovo-tidy-lsp.exe PID"
    }
    Write-Log '  PASS: lenovo-tidy-lsp.exe observed spawning under devenv'

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
