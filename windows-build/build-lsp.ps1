# Phase 4: Build lenovo-tidy-lsp.exe (Rust LSP server, MSVC ABI).

. $PSScriptRoot\helpers.ps1
Start-PhaseLog 'phase4-lsp'

try {
    Use-RustEnv
    Initialize-MsvcEnv -VsBuildToolsRoot (Join-Path $DevTools 'vs-build-tools')

    $lspDir = Join-Path $Root 'LenovoTidyLsp'
    Push-Location $lspDir
    try {
        Write-Log 'cargo build --release --target x86_64-pc-windows-msvc'
        $ec = Invoke-Native {
            cargo build --release --target x86_64-pc-windows-msvc
        }
        if ($ec -ne 0) { throw "cargo build failed: $ec" }
    } finally {
        Pop-Location
    }

    $exe = Join-Path $lspDir 'target\x86_64-pc-windows-msvc\release\lenovo-tidy-lsp.exe'
    if (-not (Test-Path $exe)) { throw "exe missing: $exe" }
    Write-Log ("Built {0} ({1:N0} bytes)" -f $exe, (Get-Item $exe).Length)

    $target = Join-Path $DistBin 'lenovo-tidy-lsp.exe'
    Copy-Item $exe $target -Force
    Write-Log "Staged -> $target"

    Stop-PhaseLog 'phase4-lsp' 0
    exit 0
} catch {
    Write-Log "FATAL: $($_.Exception.Message)" 'ERROR'
    Write-Log $_.ScriptStackTrace 'ERROR'
    Stop-PhaseLog 'phase4-lsp' 1
    exit 1
}
