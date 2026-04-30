# Phase 5: Stage binaries into LenovoTidyVscode\server-bin\win32-x64\,
# install Node deps, compile TS, package vsix using existing Node 24.

. $PSScriptRoot\helpers.ps1
Start-PhaseLog 'phase5-vscode'

try {
    $vscDir    = Join-Path $Root 'LenovoTidyVscode'
    $serverBin = Join-Path $vscDir 'server-bin\win32-x64'
    New-Item -ItemType Directory -Path $serverBin -Force | Out-Null

    $stagedDll      = Join-Path $DistBin 'LenovoTidyChecks.dll'
    $stagedLspExe   = Join-Path $DistBin 'lenovo-tidy-lsp.exe'
    $stagedTidyExe  = Join-Path $DistBin 'lenovo-clang-tidy.exe'
    foreach ($p in @($stagedDll, $stagedLspExe, $stagedTidyExe)) {
        if (-not (Test-Path $p)) { throw "Missing artifact: $p (run earlier phases first)" }
    }

    # Both the legacy dll (used on Linux/macOS) and the self-contained
    # lenovo-clang-tidy.exe (used on Windows) ship in the same folder. The
    # LSP server picks the right one at runtime based on the host platform.
    Copy-Item $stagedDll     (Join-Path $serverBin 'LenovoTidyChecks.dll') -Force
    Copy-Item $stagedLspExe  $serverBin -Force
    Copy-Item $stagedTidyExe $serverBin -Force
    Write-Log "Staged dll + 2 exes -> $serverBin"

    Push-Location $vscDir
    try {
        Write-Log 'npm install (existing Node 24 from PATH)'
        $ec = Invoke-Native { npm install --no-fund --no-audit }
        if ($ec -ne 0) { throw "npm install failed: $ec" }

        Write-Log 'TypeScript compile'
        $ec = Invoke-Native { npx --yes tsc -p ./ }
        if ($ec -ne 0) { throw "tsc failed: $ec" }

        Write-Log 'vsce package'
        $ec = Invoke-Native {
            npx --yes vsce package --out lenovo-tidy-vscode.vsix `
                --allow-missing-repository --skip-license `
                --baseContentUrl 'https://example.internal.lenovo/drj-tidy/' `
                --baseImagesUrl  'https://example.internal.lenovo/drj-tidy/'
        }
        if ($ec -ne 0) { throw "vsce package failed: $ec" }
    } finally {
        Pop-Location
    }

    $vsix = Join-Path $vscDir 'lenovo-tidy-vscode.vsix'
    if (-not (Test-Path $vsix)) { throw "vsix not produced: $vsix" }
    Copy-Item $vsix (Join-Path $DistDir 'lenovo-tidy-vscode.vsix') -Force
    Write-Log ("Staged -> {0} ({1:N0} bytes)" -f (Join-Path $DistDir 'lenovo-tidy-vscode.vsix'), (Get-Item $vsix).Length)

    Stop-PhaseLog 'phase5-vscode' 0
    exit 0
} catch {
    Write-Log "FATAL: $($_.Exception.Message)" 'ERROR'
    Write-Log $_.ScriptStackTrace 'ERROR'
    Stop-PhaseLog 'phase5-vscode' 1
    exit 1
}
