# Phase 1: Admin-elevated installer for system tools.
# - LLVM 18.1.8 (NSIS silent)
# - VS Build Tools 2022 (C++ workload + Win 11 SDK)
# - Python 3.12 embeddable + pip + lit
# - CMake 3.29 portable
# - Ninja 1.12.1
#
# Triggers ONE UAC prompt when launched via Start-Process -Verb RunAs.

. $PSScriptRoot\helpers.ps1
Start-PhaseLog 'phase1-prereqs'

if (-not (Test-Admin)) {
    Write-Log 'This script requires Administrator privileges.' 'ERROR'
    Stop-PhaseLog 'phase1-prereqs' 1
    exit 1
}

try {
    # ===== LLVM 18.1.8 =====
    $llvmExe = Join-Path $Downloads 'LLVM-18.1.8-win64.exe'
    $llvmDir = Join-Path $DevTools 'llvm18'
    if (-not (Test-Path (Join-Path $llvmDir 'bin\clang-tidy.exe'))) {
        Invoke-Download `
            -Url 'https://github.com/llvm/llvm-project/releases/download/llvmorg-18.1.8/LLVM-18.1.8-win64.exe' `
            -Destination $llvmExe
        Write-Log "Installing LLVM 18.1.8 silently to $llvmDir (~3 GB, takes ~3 min) ..."
        $proc = Start-Process -FilePath $llvmExe -ArgumentList '/S', "/D=$llvmDir" -Wait -PassThru
        if ($proc.ExitCode -ne 0) { throw "LLVM installer exit code: $($proc.ExitCode)" }
        if (-not (Test-Path (Join-Path $llvmDir 'bin\clang-tidy.exe'))) {
            throw "clang-tidy.exe not found after install"
        }
        Write-Log 'LLVM 18.1.8 installed.'
    } else {
        Write-Log 'LLVM 18 already present, skipping.'
    }

    # ===== VS Build Tools 2022 =====
    $bsExe = Join-Path $Downloads 'vs_buildtools.exe'
    $bsDir = Join-Path $DevTools 'vs-build-tools'
    $vcvars = Join-Path $bsDir 'VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path $vcvars)) {
        Invoke-Download `
            -Url 'https://aka.ms/vs/17/release/vs_buildtools.exe' `
            -Destination $bsExe
        Write-Log "Installing VS Build Tools 2022 to $bsDir (~6 GB, takes 8-12 min) ..."
        $vsArgs = @(
            '--installPath',           $bsDir,
            '--add', 'Microsoft.VisualStudio.Workload.VCTools',
            '--add', 'Microsoft.VisualStudio.Component.VC.Tools.x86.x64',
            '--add', 'Microsoft.VisualStudio.Component.Windows11SDK.22621',
            '--add', 'Microsoft.VisualStudio.Component.VC.CMake.Project',
            '--includeRecommended',
            '--quiet', '--wait', '--norestart', '--nocache'
        )
        $proc = Start-Process -FilePath $bsExe -ArgumentList $vsArgs -Wait -PassThru
        # 0 = ok, 3010 = ok but reboot suggested, 1 = warnings only
        if ($proc.ExitCode -notin @(0, 1, 3010)) {
            throw "VS Build Tools installer exit code: $($proc.ExitCode)"
        }
        if (-not (Test-Path $vcvars)) {
            throw "vcvars64.bat not found after VS BT install"
        }
        Write-Log 'VS Build Tools 2022 installed.'
    } else {
        Write-Log 'VS Build Tools already present, skipping.'
    }

    # ===== Python 3.12 embeddable + pip + lit =====
    $pyZip = Join-Path $Downloads 'python-3.12.7-embed-amd64.zip'
    $pyDir = Join-Path $DevTools 'python'
    if (-not (Test-Path (Join-Path $pyDir 'python.exe'))) {
        Invoke-Download `
            -Url 'https://www.python.org/ftp/python/3.12.7/python-3.12.7-embed-amd64.zip' `
            -Destination $pyZip
        Expand-ZipTo -Zip $pyZip -Destination $pyDir
        $pthFile = Get-ChildItem -Path $pyDir -Filter 'python*._pth' | Select-Object -First 1
        if ($pthFile) {
            $content = Get-Content $pthFile.FullName -Raw
            if ($content -notmatch '(?m)^\s*import\s+site') {
                $content = $content -replace '(?m)^#\s*import\s+site\s*$', 'import site'
                if ($content -notmatch '(?m)^\s*import\s+site') {
                    $content += "`r`nimport site`r`n"
                }
                Set-Content -Path $pthFile.FullName -Value $content -Encoding ASCII
            }
        }
        $getPip = Join-Path $Downloads 'get-pip.py'
        Invoke-Download -Url 'https://bootstrap.pypa.io/get-pip.py' -Destination $getPip
        & "$pyDir\python.exe" $getPip --no-warn-script-location
        if ($LASTEXITCODE -ne 0) { throw "get-pip.py failed: $LASTEXITCODE" }
        & "$pyDir\python.exe" -m pip install --no-warn-script-location lit
        if ($LASTEXITCODE -ne 0) { throw "pip install lit failed: $LASTEXITCODE" }
        Write-Log 'Python 3.12 + pip + lit installed.'
    } else {
        Write-Log 'Python already present, skipping.'
    }

    # ===== CMake 3.29 portable =====
    $cmZip = Join-Path $Downloads 'cmake-3.29.6-windows-x86_64.zip'
    $cmDir = Join-Path $DevTools 'cmake'
    if (-not (Test-Path (Join-Path $cmDir 'bin\cmake.exe'))) {
        Invoke-Download `
            -Url 'https://github.com/Kitware/CMake/releases/download/v3.29.6/cmake-3.29.6-windows-x86_64.zip' `
            -Destination $cmZip
        $tmpDir = Join-Path $Downloads 'cmake-extract'
        if (Test-Path $tmpDir) { Remove-Item -Recurse -Force $tmpDir }
        Expand-ZipTo -Zip $cmZip -Destination $tmpDir
        $inner = Get-ChildItem $tmpDir -Directory | Select-Object -First 1
        if (Test-Path $cmDir) { Remove-Item -Recurse -Force $cmDir }
        Move-Item $inner.FullName $cmDir
        Remove-Item -Recurse -Force $tmpDir
        Write-Log 'CMake 3.29.6 installed.'
    } else {
        Write-Log 'CMake already present, skipping.'
    }

    # ===== Ninja =====
    $nZip = Join-Path $Downloads 'ninja-win.zip'
    $nDir = Join-Path $DevTools 'ninja'
    if (-not (Test-Path (Join-Path $nDir 'ninja.exe'))) {
        Invoke-Download `
            -Url 'https://github.com/ninja-build/ninja/releases/download/v1.12.1/ninja-win.zip' `
            -Destination $nZip
        Expand-ZipTo -Zip $nZip -Destination $nDir
        Write-Log 'Ninja 1.12.1 installed.'
    } else {
        Write-Log 'Ninja already present, skipping.'
    }

    Write-Log '----- Final tool versions -----'
    & "$llvmDir\bin\clang-tidy.exe" --version 2>&1 | ForEach-Object { Write-Log $_ }
    & "$cmDir\bin\cmake.exe" --version 2>&1 | Select-Object -First 1 | ForEach-Object { Write-Log $_ }
    & "$nDir\ninja.exe" --version 2>&1 | ForEach-Object { Write-Log "ninja: $_" }
    & "$pyDir\python.exe" --version 2>&1 | ForEach-Object { Write-Log $_ }
    & "$pyDir\python.exe" -m lit --version 2>&1 | ForEach-Object { Write-Log "lit: $_" }

    Stop-PhaseLog 'phase1-prereqs' 0
    exit 0
} catch {
    Write-Log "FATAL: $($_.Exception.Message)" 'ERROR'
    Write-Log $_.ScriptStackTrace 'ERROR'
    Stop-PhaseLog 'phase1-prereqs' 1
    exit 1
}
