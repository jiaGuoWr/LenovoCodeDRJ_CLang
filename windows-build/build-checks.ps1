# Phase 3: Build LenovoTidyChecks.dll (clang-tidy plugin) via CMake + Ninja + cl.exe.

. $PSScriptRoot\helpers.ps1
Start-PhaseLog 'phase3-checks'

try {
    Use-ToolchainEnv
    Initialize-MsvcEnv -VsBuildToolsRoot (Join-Path $DevTools 'vs-build-tools')

    $checksDir = Join-Path $Root 'LenovoTidyChecks'
    $buildDir  = Join-Path $checksDir 'build\windows-release'

    if (Test-Path $buildDir) {
        Write-Log "Cleaning previous build dir: $buildDir"
        Remove-Item -Recurse -Force $buildDir
    }

    Push-Location $checksDir
    try {
        Write-Log 'Configure CMake (Ninja + cl.exe + LLVM 18 dev libs)'
        $cfgEc = Invoke-Native {
            cmake -S . -B $buildDir -G Ninja `
                -DCMAKE_BUILD_TYPE=RelWithDebInfo `
                -DCMAKE_C_COMPILER=cl `
                -DCMAKE_CXX_COMPILER=cl `
                -DLENOVO_TIDY_BUILD_TESTS=OFF `
                "-DLLVM_DIR=$env:LLVM_DIR" `
                "-DClang_DIR=$env:Clang_DIR"
        }
        if ($cfgEc -ne 0) { throw "cmake configure failed: $cfgEc" }

        Write-Log 'Building with Ninja ...'
        $bldEc = Invoke-Native { cmake --build $buildDir -j }
        if ($bldEc -ne 0) { throw "cmake build failed: $bldEc" }
    } finally {
        Pop-Location
    }

    $dll = Get-ChildItem -Path $buildDir -Filter '*LenovoTidyChecks*.dll' -Recurse |
        Select-Object -First 1
    if (-not $dll) { throw "LenovoTidyChecks dll not found under $buildDir" }
    Write-Log ("Built {0} ({1:N0} bytes)" -f $dll.FullName, $dll.Length)

    $target = Join-Path $DistBin 'LenovoTidyChecks.dll'
    Copy-Item $dll.FullName $target -Force
    Write-Log "Staged -> $target"

    Stop-PhaseLog 'phase3-checks' 0
    exit 0
} catch {
    Write-Log "FATAL: $($_.Exception.Message)" 'ERROR'
    Write-Log $_.ScriptStackTrace 'ERROR'
    Stop-PhaseLog 'phase3-checks' 1
    exit 1
}
