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
    # registered VS IDE; manually assemble the vsix from build outputs. -----
    $buildOut = Join-Path $vsDir 'bin\x64\Release\net472'
    if (-not (Test-Path (Join-Path $buildOut 'LenovoTidy.dll'))) {
        $buildOut = Join-Path $vsDir 'bin\Release\net472'
    }
    if (-not (Test-Path (Join-Path $buildOut 'LenovoTidy.dll'))) {
        throw "LenovoTidy.dll not found under $vsDir\bin"
    }
    Write-Log "Build output: $buildOut"

    $stage = Join-Path $vsDir 'obj\vsix-stage'
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage }
    New-Item -ItemType Directory -Path $stage -Force | Out-Null

    # Copy primary assembly + dependencies that are NOT shipped by VS itself.
    Copy-Item (Join-Path $buildOut 'LenovoTidy.dll')      $stage -Force
    Copy-Item (Join-Path $buildOut 'LenovoTidy.pdb')      $stage -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $buildOut 'Newtonsoft.Json.dll') $stage -ErrorAction SilentlyContinue
    Copy-Item (Join-Path $vsDir   'LICENSE.txt')          $stage -Force
    Copy-Item (Join-Path $vsDir   'README.md')            $stage -ErrorAction SilentlyContinue

    # ----- Generate the *deployment* extension.vsixmanifest -----
    # source.extension.vsixmanifest is the design-time template VSSDK normally
    # post-processes during MSBuild. Since `dotnet build` does not run those
    # targets reliably without a full VS IDE, we replicate the four key
    # substitutions ourselves:
    #   1. drop `xmlns:d="...vsx-schema-design/2011"` namespace declaration;
    #   2. drop every `d:Source=...` attribute;
    #   3. drop every `d:ProjectName=...` attribute;
    #   4. expand `Path="|ProjectName|"` placeholders to "ProjectName.dll".
    # Without these, the VSIX installer rejects the package with
    # `InvalidExtensionPackageException: this file is not a valid VSIX package`.
    $manifestSrc = Get-Content (Join-Path $vsDir 'source.extension.vsixmanifest') -Raw
    $manifestDst = $manifestSrc `
        -replace '\s+xmlns:d="[^"]*"', '' `
        -replace '\s+d:Source="[^"]*"', '' `
        -replace '\s+d:ProjectName="[^"]*"', '' `
        -replace 'Path="\|([^|]+)\|"', 'Path="$1.dll"'
    [System.IO.File]::WriteAllText(
        (Join-Path $stage 'extension.vsixmanifest'),
        $manifestDst,
        (New-Object System.Text.UTF8Encoding $false))

    Copy-Item -Recurse (Join-Path $buildOut 'server-bin') (Join-Path $stage 'server-bin') -Force

    # ----- [Content_Types].xml (VSIX v3 minimal) -----
    # Format mirrors what `dotnet build` of a VS 2022 VSIXProject produces.
    # No `<?xml ?>` declaration; one line; covers every extension we ship.
    $ctxml = '<?xml version="1.0" encoding="utf-8"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="vsixmanifest" ContentType="text/xml" /><Default Extension="dll" ContentType="application/octet-stream" /><Default Extension="exe" ContentType="application/octet-stream" /><Default Extension="pdb" ContentType="application/octet-stream" /><Default Extension="json" ContentType="application/json" /><Default Extension="txt" ContentType="text/plain" /><Default Extension="md" ContentType="text/markdown" /></Types>'
    [System.IO.File]::WriteAllText((Join-Path $stage '[Content_Types].xml'), $ctxml, (New-Object System.Text.UTF8Encoding $false))

    # ----- catalog.json + manifest.json (VSIX v3, required since VS 2017) -----
    # Without these files the modern VSIXInstaller refuses with
    # `InvalidExtensionPackageException: this file is not a valid VSIX package`.
    # `_rels/.rels` (the legacy OPC relationship file) is NOT used in v3 and
    # we deliberately do not emit it.
    $vsixId      = 'LenovoTidy.VS2022.f89e3441'
    $vsixVersion = '0.2.0'
    $extDirName  = 'lenovotidy.lenovo'  # any unique short name; VS overwrites

    # File list for manifest.json
    $stageFull = (Resolve-Path $stage).Path
    $files = @()
    foreach ($f in Get-ChildItem $stage -Recurse -File) {
        $rel = $f.FullName.Substring($stageFull.Length).TrimStart('\','/').Replace('\','/')
        # Skip the metadata files themselves
        if ($rel -in @('catalog.json','manifest.json','[Content_Types].xml')) { continue }
        $files += @{ fileName = "/$rel"; sha256 = $null }
    }

    $catalog = [ordered]@{
        manifestVersion = '1.1'
        info = [ordered]@{
            id           = "$vsixId,version=$vsixVersion"
            manifestType = 'Extension'
        }
        packages = @(
            [ordered]@{
                id           = "Component.$vsixId"
                version      = $vsixVersion
                type         = 'Component'
                extension    = $true
                dependencies = [ordered]@{
                    "$vsixId" = $vsixVersion
                    'Microsoft.VisualStudio.Component.CoreEditor' = '[17.0,18.0)'
                }
                localizedResources = @(
                    [ordered]@{
                        language    = 'en-US'
                        title       = 'Lenovo Tidy'
                        description = 'Lenovo DRJ custom Clang-Tidy rules for C++ (LSP-powered).'
                    }
                )
            },
            [ordered]@{
                id           = $vsixId
                version      = $vsixVersion
                type         = 'Vsix'
                payloads     = @(
                    [ordered]@{ fileName = 'LenovoTidy.vsix'; size = 0 }
                )
                vsixId       = $vsixId
                extensionDir = "[installdir]\Common7\IDE\Extensions\$extDirName"
                installSizes = [ordered]@{ targetDrive = 0 }
            }
        )
    }
    $catalogJson = $catalog | ConvertTo-Json -Depth 10 -Compress
    [System.IO.File]::WriteAllText((Join-Path $stage 'catalog.json'), $catalogJson, (New-Object System.Text.UTF8Encoding $false))

    $manifest = [ordered]@{
        id           = $vsixId
        version      = $vsixVersion
        type         = 'Vsix'
        vsixId       = $vsixId
        extensionDir = "[installdir]\Common7\IDE\Extensions\$extDirName"
        files        = $files
        installSizes = [ordered]@{ targetDrive = 0 }
        dependencies = [ordered]@{
            'Microsoft.VisualStudio.Component.CoreEditor' = '[17.0,18.0)'
        }
    }
    $manifestJson = $manifest | ConvertTo-Json -Depth 10 -Compress
    [System.IO.File]::WriteAllText((Join-Path $stage 'manifest.json'), $manifestJson, (New-Object System.Text.UTF8Encoding $false))

    Write-Log ("Generated catalog.json + manifest.json (files listed: {0})" -f $files.Count)

    $vsixPath = Join-Path $vsDir 'bin\Release\LenovoTidy.vsix'
    New-Item -ItemType Directory -Path (Split-Path $vsixPath) -Force | Out-Null
    if (Test-Path $vsixPath) { Remove-Item $vsixPath -Force }

    # Manually walk the staging directory so we can guarantee forward-slash
    # entry names. ZipFile.CreateFromDirectory on .NET Framework keeps the
    # Windows backslash in the archive, which makes the VSIX installer reject
    # the package because its file paths don't match catalog.json's entries.
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    Add-Type -AssemblyName System.IO.Compression
    $zipStream = [System.IO.File]::Open($vsixPath, 'Create')
    $zipArchive = New-Object System.IO.Compression.ZipArchive($zipStream, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        $stageFull = (Resolve-Path $stage).Path
        foreach ($file in Get-ChildItem $stage -Recurse -File) {
            $relWin = $file.FullName.Substring($stageFull.Length).TrimStart('\','/')
            $entryName = $relWin -replace '\\','/'
            [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $zipArchive, $file.FullName, $entryName,
                [System.IO.Compression.CompressionLevel]::Optimal)
        }
    } finally {
        $zipArchive.Dispose()
        $zipStream.Dispose()
    }

    if (-not (Test-Path $vsixPath)) { throw "Failed to assemble $vsixPath" }
    $vsixSize = (Get-Item $vsixPath).Length
    Write-Log ("Assembled vsix: {0} ({1:N0} bytes)" -f $vsixPath, $vsixSize)

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
