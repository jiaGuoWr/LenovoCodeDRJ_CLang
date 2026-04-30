# Phase 2: Install Rust via rustup-init (no UAC, per-user install at D:\dev-tools\rust).

. $PSScriptRoot\helpers.ps1
Start-PhaseLog 'phase2-rust'

try {
    Use-RustEnv
    New-Item -ItemType Directory -Path $env:RUSTUP_HOME -Force | Out-Null
    New-Item -ItemType Directory -Path $env:CARGO_HOME  -Force | Out-Null

    $cargoExe = Join-Path $env:CARGO_HOME 'bin\cargo.exe'
    if (-not (Test-Path $cargoExe)) {
        $rustup = Join-Path $Downloads 'rustup-init.exe'
        Invoke-Download `
            -Url 'https://static.rust-lang.org/rustup/dist/x86_64-pc-windows-msvc/rustup-init.exe' `
            -Destination $rustup
        Write-Log 'Running rustup-init -y --default-toolchain stable-x86_64-pc-windows-msvc --profile minimal --no-modify-path'
        & $rustup -y --default-toolchain stable-x86_64-pc-windows-msvc --profile minimal --no-modify-path 2>&1 |
            ForEach-Object { Write-Log $_ }
        if ($LASTEXITCODE -ne 0) { throw "rustup-init exit: $LASTEXITCODE" }
        if (-not (Test-Path $cargoExe)) {
            throw "cargo.exe missing after rustup-init"
        }
    } else {
        Write-Log 'Rust toolchain already installed, skipping.'
    }

    Write-Log '----- Rust toolchain versions -----'
    & "$env:CARGO_HOME\bin\rustup.exe" --version 2>&1 | ForEach-Object { Write-Log $_ }
    & "$env:CARGO_HOME\bin\rustc.exe"  --version 2>&1 | ForEach-Object { Write-Log $_ }
    & "$env:CARGO_HOME\bin\cargo.exe"  --version 2>&1 | ForEach-Object { Write-Log $_ }
    & "$env:CARGO_HOME\bin\rustup.exe" target list --installed 2>&1 | ForEach-Object { Write-Log "target: $_" }

    Stop-PhaseLog 'phase2-rust' 0
    exit 0
} catch {
    Write-Log "FATAL: $($_.Exception.Message)" 'ERROR'
    Write-Log $_.ScriptStackTrace 'ERROR'
    Stop-PhaseLog 'phase2-rust' 1
    exit 1
}
