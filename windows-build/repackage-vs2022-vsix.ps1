<#
.SYNOPSIS
    Re-packages an MSBuild-produced LenovoTidy.dll into a VS 2022 17.x VSIX
    that VSIXInstaller actually accepts.

.DESCRIPTION
    The VSSDK MSBuild targets only assemble a valid VSIX when the build
    happens *inside* a registered VS IDE (devenv build). Plain `dotnet
    build` or headless `msbuild` skip the targets that:
      1. expand `Path="|ProjectName|"` placeholders in the manifest,
      2. drop the design-time `xmlns:d` / `d:Source` / `d:ProjectName`
         attributes,
      3. emit `[Content_Types].xml`, `catalog.json`, `manifest.json`
         (the v3 metadata files VS 2017+ expects),
      4. ZIP everything with forward-slash entry names (Windows backslash
         entries make VS reject the package with
         InvalidExtensionPackageException).

    This script reproduces all four steps deterministically so the same
    output works in CI (windows-2022 hosted runner with no VS IDE) and on
    a developer's machine.

.PARAMETER VsProjectDir
    Source LenovoTidyVs2022/ directory (contains source.extension.vsixmanifest).

.PARAMETER BuildOutputDir
    MSBuild output directory containing LenovoTidy.dll, LenovoTidy.pdb,
    and (optionally) Newtonsoft.Json.dll.

.PARAMETER ServerBinDir
    Directory containing server-bin/win32-x64/{lenovo-tidy-lsp.exe,
    lenovo-clang-tidy.exe, LenovoTidyChecks.dll}. Will be copied into the
    VSIX under server-bin/win32-x64/. Pass an empty string to skip (the
    resulting VSIX will be unable to spawn the LSP).

.PARAMETER OutVsix
    Final .vsix path to produce.

.PARAMETER VsixId
    VSIX identity id (matches Identity Id= in source.extension.vsixmanifest).

.PARAMETER VsixVersion
    VSIX semver version string.

.EXAMPLE
    pwsh repackage-vs2022-vsix.ps1 `
        -VsProjectDir   D:\LenovoDRJ_CLang\LenovoTidyVs2022 `
        -BuildOutputDir D:\LenovoDRJ_CLang\LenovoTidyVs2022\bin\x64\Release\net472 `
        -ServerBinDir   D:\LenovoDRJ_CLang\LenovoTidyVs2022 `
        -OutVsix        D:\LenovoDRJ_CLang\dist\LenovoTidy.vsix `
        -VsixId         LenovoTidy.VS2022.f89e3441 `
        -VsixVersion    0.2.0
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string] $VsProjectDir,
    [Parameter(Mandatory)] [string] $BuildOutputDir,
    [string] $ServerBinDir = '',
    [Parameter(Mandatory)] [string] $OutVsix,
    [Parameter(Mandatory)] [string] $VsixId,
    [Parameter(Mandatory)] [string] $VsixVersion,
    [string] $ExtensionDirName = 'lenovotidy.lenovo'
)

$ErrorActionPreference = 'Stop'

function Resolve-RequiredPath {
    param([Parameter(Mandatory)] [string] $Path, [string] $Label)
    if (-not (Test-Path $Path)) {
        throw "$Label not found: $Path"
    }
    return (Resolve-Path $Path).Path
}

$VsProjectDir   = Resolve-RequiredPath $VsProjectDir   'VsProjectDir'
$BuildOutputDir = Resolve-RequiredPath $BuildOutputDir 'BuildOutputDir'
$dll = Join-Path $BuildOutputDir 'LenovoTidy.dll'
if (-not (Test-Path $dll)) { throw "LenovoTidy.dll missing under $BuildOutputDir" }

$stage = Join-Path $env:TEMP ("lenovo-vsix-stage-" + [Guid]::NewGuid())
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Write-Host "  Staging at: $stage"

try {
    # ----- 1) Copy primary assembly + payload deps that VS does NOT ship ---
    Copy-Item $dll                                                 $stage -Force
    foreach ($maybe in @('LenovoTidy.pdb', 'Newtonsoft.Json.dll')) {
        $src = Join-Path $BuildOutputDir $maybe
        if (Test-Path $src) { Copy-Item $src $stage -Force }
    }
    foreach ($maybe in @('LICENSE.txt', 'README.md')) {
        $src = Join-Path $VsProjectDir $maybe
        if (Test-Path $src) { Copy-Item $src $stage -Force }
    }

    # ----- 2) Copy server-bin (LSP exe + clang-tidy + plugin dll) ---------
    if ($ServerBinDir -and (Test-Path (Join-Path $ServerBinDir 'server-bin'))) {
        Copy-Item -Recurse (Join-Path $ServerBinDir 'server-bin') `
                  (Join-Path $stage 'server-bin') -Force
    } else {
        Write-Warning '  ServerBinDir/server-bin not provided; the VSIX will be inert (no LSP).'
        New-Item -ItemType Directory -Path (Join-Path $stage 'server-bin\win32-x64') -Force | Out-Null
    }

    # ----- 3) Generate the deployment extension.vsixmanifest --------------
    # Replicates the four substitutions VSSDK normally does at build time:
    #   - drop xmlns:d="..."    (design-time namespace)
    #   - drop d:Source="..."   (project-time markers)
    #   - drop d:ProjectName="..."
    #   - expand Path="|Project|" placeholders to "Project.dll"
    # Without these the VSIX installer rejects the package with
    # InvalidExtensionPackageException ("not a valid VSIX package").
    $manifestSrcPath = Join-Path $VsProjectDir 'source.extension.vsixmanifest'
    if (-not (Test-Path $manifestSrcPath)) {
        throw "source.extension.vsixmanifest missing under $VsProjectDir"
    }
    $manifestSrc = Get-Content $manifestSrcPath -Raw
    $manifestDst = $manifestSrc `
        -replace '\s+xmlns:d="[^"]*"', '' `
        -replace '\s+d:Source="[^"]*"', '' `
        -replace '\s+d:ProjectName="[^"]*"', '' `
        -replace 'Path="\|([^|]+)\|"', 'Path="$1.dll"'
    [System.IO.File]::WriteAllText(
        (Join-Path $stage 'extension.vsixmanifest'),
        $manifestDst,
        (New-Object System.Text.UTF8Encoding $false))

    # ----- 4) [Content_Types].xml (VSIX v3 minimal) ----------------------
    # Format mirrors what `dotnet build` of a VS 2022 VSIXProject produces
    # when the VS IDE *is* present. One line, no XML declaration spacing,
    # covers every extension we ship.
    $ctxml = '<?xml version="1.0" encoding="utf-8"?><Types xmlns="http://schemas.openxmlformats.org/package/2006/content-types"><Default Extension="vsixmanifest" ContentType="text/xml" /><Default Extension="dll" ContentType="application/octet-stream" /><Default Extension="exe" ContentType="application/octet-stream" /><Default Extension="pdb" ContentType="application/octet-stream" /><Default Extension="json" ContentType="application/json" /><Default Extension="txt" ContentType="text/plain" /><Default Extension="md" ContentType="text/markdown" /></Types>'
    [System.IO.File]::WriteAllText(
        (Join-Path $stage '[Content_Types].xml'),
        $ctxml,
        (New-Object System.Text.UTF8Encoding $false))

    # ----- 5) catalog.json + manifest.json (required since VS 2017) ------
    # Without these files the modern VSIXInstaller refuses to install with
    # InvalidExtensionPackageException. _rels/.rels (legacy OPC) is NOT
    # used in v3 and we deliberately do not emit it.
    $stageFull = (Resolve-Path $stage).Path
    $files = @()
    foreach ($f in Get-ChildItem $stage -Recurse -File) {
        $rel = $f.FullName.Substring($stageFull.Length).TrimStart('\','/').Replace('\','/')
        if ($rel -in @('catalog.json','manifest.json','[Content_Types].xml')) { continue }
        $files += @{ fileName = "/$rel"; sha256 = $null }
    }

    $catalog = [ordered]@{
        manifestVersion = '1.1'
        info = [ordered]@{
            id           = "$VsixId,version=$VsixVersion"
            manifestType = 'Extension'
        }
        packages = @(
            [ordered]@{
                id           = "Component.$VsixId"
                version      = $VsixVersion
                type         = 'Component'
                extension    = $true
                dependencies = [ordered]@{
                    "$VsixId" = $VsixVersion
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
                id           = $VsixId
                version      = $VsixVersion
                type         = 'Vsix'
                payloads     = @(
                    [ordered]@{ fileName = 'LenovoTidy.vsix'; size = 0 }
                )
                vsixId       = $VsixId
                extensionDir = "[installdir]\Common7\IDE\Extensions\$ExtensionDirName"
                installSizes = [ordered]@{ targetDrive = 0 }
            }
        )
    }
    [System.IO.File]::WriteAllText(
        (Join-Path $stage 'catalog.json'),
        ($catalog | ConvertTo-Json -Depth 10 -Compress),
        (New-Object System.Text.UTF8Encoding $false))

    $manifest = [ordered]@{
        id           = $VsixId
        version      = $VsixVersion
        type         = 'Vsix'
        vsixId       = $VsixId
        extensionDir = "[installdir]\Common7\IDE\Extensions\$ExtensionDirName"
        files        = $files
        installSizes = [ordered]@{ targetDrive = 0 }
        dependencies = [ordered]@{
            'Microsoft.VisualStudio.Component.CoreEditor' = '[17.0,18.0)'
        }
    }
    [System.IO.File]::WriteAllText(
        (Join-Path $stage 'manifest.json'),
        ($manifest | ConvertTo-Json -Depth 10 -Compress),
        (New-Object System.Text.UTF8Encoding $false))

    Write-Host ("  Generated catalog.json + manifest.json (files listed: {0})" -f $files.Count)

    # ----- 6) Zip with forward-slash entry names ------------------------
    # ZipFile.CreateFromDirectory on .NET Framework 4.x keeps Windows
    # backslashes in the archive header. The VSIX installer compares
    # entry names byte-for-byte against catalog.json's forward-slash
    # paths and rejects a backslash-mismatched archive. We walk the
    # staging tree manually to guarantee correctness.
    $outDir = Split-Path $OutVsix -Parent
    if ($outDir -and -not (Test-Path $outDir)) {
        New-Item -ItemType Directory -Path $outDir -Force | Out-Null
    }
    if (Test-Path $OutVsix) { Remove-Item $OutVsix -Force }

    Add-Type -AssemblyName System.IO.Compression.FileSystem
    Add-Type -AssemblyName System.IO.Compression
    $zipStream  = [System.IO.File]::Open($OutVsix, 'Create')
    $zipArchive = New-Object System.IO.Compression.ZipArchive(
        $zipStream, [System.IO.Compression.ZipArchiveMode]::Create)
    try {
        foreach ($file in Get-ChildItem $stage -Recurse -File) {
            $relWin    = $file.FullName.Substring($stageFull.Length).TrimStart('\','/')
            $entryName = $relWin -replace '\\', '/'
            [void][System.IO.Compression.ZipFileExtensions]::CreateEntryFromFile(
                $zipArchive, $file.FullName, $entryName,
                [System.IO.Compression.CompressionLevel]::Optimal)
        }
    } finally {
        $zipArchive.Dispose()
        $zipStream.Dispose()
    }

    if (-not (Test-Path $OutVsix)) { throw "Failed to assemble $OutVsix" }
    $size = (Get-Item $OutVsix).Length
    Write-Host ("  Assembled vsix: {0} ({1:N0} bytes)" -f $OutVsix, $size)
}
finally {
    if (Test-Path $stage) { Remove-Item -Recurse -Force $stage -EA 0 }
}
