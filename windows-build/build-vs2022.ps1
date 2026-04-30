# Phase 6: Stage binaries into LenovoTidyVs2022\server-bin\win32-x64\,
# patch csproj to use Microsoft.NETFramework.ReferenceAssemblies.net472,
# dotnet restore + MSBuild Release -> LenovoTidy.vsix.

. $PSScriptRoot\helpers.ps1
Start-PhaseLog 'phase6-vs2022'

try {
    Initialize-MsvcEnv -VsBuildToolsRoot (Join-Path $DevTools 'vs-build-tools')

    $vsDir     = Join-Path $Root 'LenovoTidyVs2022'
    $serverBin = Join-Path $vsDir 'server-bin\win32-x64'
    New-Item -ItemType Directory -Path $serverBin -Force | Out-Null

    Copy-Item (Join-Path $DistBin 'LenovoTidyChecks.dll')   $serverBin -Force
    Copy-Item (Join-Path $DistBin 'lenovo-tidy-lsp.exe')    $serverBin -Force
    Copy-Item (Join-Path $DistBin 'lenovo-clang-tidy.exe')  $serverBin -Force
    Write-Log "Staged dll + 2 exes -> $serverBin"

    # ----- Patch csproj (idempotent) -----
    $csproj  = Join-Path $vsDir 'LenovoTidy.csproj'
    $content = Get-Content $csproj -Raw
    if ($content -notmatch 'Microsoft\.NETFramework\.ReferenceAssemblies') {
        Write-Log "Patching $csproj : add ReferenceAssemblies.net472 PackageReference"
        $newRef = '    <PackageReference Include="Microsoft.NETFramework.ReferenceAssemblies.net472" Version="1.0.3" PrivateAssets="all" />'
        $idx = $content.LastIndexOf('</ItemGroup>')
        if ($idx -lt 0) { throw "could not find </ItemGroup> in csproj" }
        $patched = $content.Substring(0, $idx) + $newRef + "`r`n  " + $content.Substring($idx)
        Set-Content -Path $csproj -Value $patched -Encoding UTF8 -NoNewline
        Write-Log 'csproj patched.'
    } else {
        Write-Log 'csproj already references Net472 ReferenceAssemblies, skipping patch.'
    }

    # ----- Locate MSBuild.exe in VS BT -----
    $msbuild = Get-ChildItem -Path (Join-Path $DevTools 'vs-build-tools\MSBuild') `
        -Filter 'MSBuild.exe' -Recurse -ErrorAction SilentlyContinue |
        Where-Object { $_.FullName -match '\\Bin\\(amd64\\)?MSBuild\.exe$' } |
        Sort-Object { $_.FullName.Length } | Select-Object -First 1
    if (-not $msbuild) { throw "MSBuild.exe not found under VS Build Tools" }
    Write-Log "MSBuild: $($msbuild.FullName)"

    $nugetConfig = Join-Path $WindowsBuild 'nuget.config'
    if (-not (Test-Path $nugetConfig)) { throw "Local nuget.config missing: $nugetConfig" }

    Push-Location $vsDir
    try {
        Write-Log "dotnet restore (--configfile $nugetConfig)"
        $ec = Invoke-Native {
            dotnet restore LenovoTidy.csproj --configfile $nugetConfig
        }
        if ($ec -ne 0) { throw "dotnet restore failed: $ec" }

        Write-Log 'dotnet build (uses bundled SDK + VSSDK.BuildTools targets)'
        $ec = Invoke-Native {
            dotnet build LenovoTidy.csproj `
                --configuration Release `
                --no-restore `
                /p:DeployExtension=false `
                /p:CreateVsixContainer=true `
                --verbosity minimal
        }
        if ($ec -ne 0) { throw "dotnet build failed: $ec" }
    } finally {
        Pop-Location
    }

    # ----- VSSDK targets do not run reliably under "dotnet build" without a
    # registered VS IDE; delegate the rest to repackage-vs2022-vsix.ps1
    # which produces a valid VSIX v3 from the build output. The same script
    # is invoked directly from CI (.github/workflows/release.yml) so the
    # local and CI artefacts are byte-equivalent. -----
    $buildOut = Join-Path $vsDir 'bin\x64\Release\net472'
    if (-not (Test-Path (Join-Path $buildOut 'LenovoTidy.dll'))) {
        $buildOut = Join-Path $vsDir 'bin\Release\net472'
    }
    if (-not (Test-Path (Join-Path $buildOut 'LenovoTidy.dll'))) {
        throw "LenovoTidy.dll not found under $vsDir\bin"
    }
    Write-Log "Build output: $buildOut"

    $vsixPath = Join-Path $vsDir 'bin\Release\LenovoTidy.vsix'
    & (Join-Path $WindowsBuild 'repackage-vs2022-vsix.ps1') `
        -VsProjectDir   $vsDir `
        -BuildOutputDir $buildOut `
        -ServerBinDir   $vsDir `
        -OutVsix        $vsixPath `
        -VsixId         'LenovoTidy.VS2022.f89e3441' `
        -VsixVersion    '0.2.0'
    if ($LASTEXITCODE -ne 0) { throw "repackage-vs2022-vsix.ps1 failed: $LASTEXITCODE" }

    Copy-Item $vsixPath (Join-Path $DistDir 'LenovoTidy.vsix') -Force
    Write-Log ("Staged -> {0}" -f (Join-Path $DistDir 'LenovoTidy.vsix'))

    Stop-PhaseLog 'phase6-vs2022' 0
    exit 0
} catch {
    Write-Log "FATAL: $($_.Exception.Message)" 'ERROR'
    Write-Log $_.ScriptStackTrace 'ERROR'
    Stop-PhaseLog 'phase6-vs2022' 1
    exit 1
}
