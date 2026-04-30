# Phase 1.5: Replace D:\dev-tools\llvm18 with the full LLVM 18.1.8 SDK
# (clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz, ~959 MB) which contains
# LLVMConfig.cmake / ClangConfig.cmake / clang*.lib needed for plugin builds.
# The official LLVM-18.1.8-win64.exe NSIS installer ships ONLY runtime tools,
# no dev libraries.

. $PSScriptRoot\helpers.ps1
Start-PhaseLog 'phase1b-llvm-dev'

try {
    $url = 'https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz'
    $tar = Join-Path $Downloads 'clang+llvm-18.1.8-x86_64-pc-windows-msvc.tar.xz'
    $extractDir = Join-Path $Downloads 'llvm-extract'
    $finalDir = Join-Path $DevTools 'llvm18'

    Invoke-Download -Url $url -Destination $tar

    if (Test-Path $extractDir) {
        Write-Log "Cleaning previous extract dir: $extractDir"
        Remove-Item -Recurse -Force $extractDir
    }
    New-Item -ItemType Directory -Path $extractDir -Force | Out-Null

    Write-Log "Extracting $tar -> $extractDir (this takes 3-6 min)"
    $tarExe = (Get-Command tar.exe -ErrorAction SilentlyContinue).Source
    if (-not $tarExe) { $tarExe = 'C:\Windows\System32\tar.exe' }
    if (-not (Test-Path $tarExe)) { throw "tar.exe not found" }

    $ec = Invoke-Native {
        & $tarExe -xf $tar -C $extractDir
    }
    if ($ec -ne 0) { throw "tar -xf failed: $ec" }

    $inner = Get-ChildItem $extractDir -Directory | Select-Object -First 1
    if (-not $inner) { throw "No inner directory in tarball" }
    Write-Log "Tarball top-level: $($inner.FullName)"

    if (Test-Path $finalDir) {
        Write-Log "Removing existing $finalDir"
        Remove-Item -Recurse -Force $finalDir
    }
    Move-Item $inner.FullName $finalDir
    Remove-Item -Recurse -Force $extractDir

    Write-Log "Verifying dev artifacts ..."
    $checks = @(
        'bin\clang-tidy.exe',
        'lib\cmake\llvm\LLVMConfig.cmake',
        'lib\cmake\clang\ClangConfig.cmake',
        'lib\clangAST.lib',
        'lib\clangTidy.lib',
        'lib\LLVMSupport.lib',
        'include\llvm\ADT\StringRef.h',
        'include\clang\AST\ASTContext.h'
    )
    foreach ($rel in $checks) {
        $p = Join-Path $finalDir $rel
        if (Test-Path $p) {
            Write-Log "  OK : $p"
        } else {
            Write-Log "  MISSING : $p" 'ERROR'
            throw "Required file missing after extract: $rel"
        }
    }

    & "$finalDir\bin\clang-tidy.exe" --version 2>&1 | ForEach-Object { Write-Log $_ }

    # ---- Patch LLVMExports.cmake to fix the hard-coded VS 2019 Pro path ----
    # The official LLVM 18.1.8 Windows tarball was built on a machine with
    # Visual Studio 2019 Professional installed; CMake captured the absolute
    # path to its DIA SDK into LLVMExports.cmake. On any other machine that
    # path will not exist and ninja will fail with:
    #   ninja: error: '.../2019/Professional/DIA SDK/lib/amd64/diaguids.lib',
    #     needed by 'LenovoTidyChecks.dll', missing.
    # We rewrite the path to the DIA SDK that ships with our VS Build Tools.
    $exportsFile = Join-Path $finalDir 'lib\cmake\llvm\LLVMExports.cmake'
    $vsBtDia = Join-Path $DevTools 'vs-build-tools\DIA SDK\lib\amd64\diaguids.lib'
    $badPath = 'C:/Program Files (x86)/Microsoft Visual Studio/2019/Professional/DIA SDK/lib/amd64/diaguids.lib'
    if (Test-Path $exportsFile) {
        $exportsContent = Get-Content $exportsFile -Raw
        if ($exportsContent.Contains($badPath)) {
            $newPath = ($vsBtDia -replace '\\','/')
            $exportsContent = $exportsContent.Replace($badPath, $newPath)
            Set-Content -Path $exportsFile -Value $exportsContent -Encoding UTF8 -NoNewline
            Write-Log "Patched LLVMExports.cmake DIA SDK path -> $newPath"
        } else {
            Write-Log 'LLVMExports.cmake DIA SDK path already patched (or never present), skipping.'
        }
    }

    Stop-PhaseLog 'phase1b-llvm-dev' 0
    exit 0
} catch {
    Write-Log "FATAL: $($_.Exception.Message)" 'ERROR'
    Write-Log $_.ScriptStackTrace 'ERROR'
    Stop-PhaseLog 'phase1b-llvm-dev' 1
    exit 1
}
