# Read-only diagnostic for VS 2022 Lenovo Tidy state
$ErrorActionPreference = 'Continue'

# Auto-discover the VS 17.0 instance directory (pattern is "17.0_<hash>",
# e.g. 17.0_ec695be7). Pick the most recently modified non-Exp instance
# so re-running on a fresh dev machine "just works" without editing the
# script. Honour LENOVO_VS_INSTANCE override for split-config setups.
$base = $env:LENOVO_VS_INSTANCE
if (-not $base) {
    $base = Get-ChildItem (Join-Path $env:LOCALAPPDATA 'Microsoft\VisualStudio') `
                -Directory -EA 0 |
            Where-Object { $_.Name -match '^17\.0_[a-f0-9]+$' } |
            Sort-Object LastWriteTime -Descending |
            Select-Object -First 1 -ExpandProperty FullName
}
if (-not $base -or -not (Test-Path $base)) {
    throw "Could not locate a VS 17.0 instance under $env:LOCALAPPDATA\Microsoft\VisualStudio. Set LENOVO_VS_INSTANCE to override."
}
Write-Host "Using VS instance: $base"

# Same auto-discovery for the install root: prefer LENOVO_VS_ROOT, else
# the standard layout under either D:\Program Files\... or
# C:\Program Files\... (whichever exists first).
$vsRoot = $env:LENOVO_VS_ROOT
if (-not $vsRoot) {
    foreach ($candidate in @(
        'D:\Program Files\Microsoft Visual Studio\2022\Community',
        'C:\Program Files\Microsoft Visual Studio\2022\Community',
        'D:\Program Files\Microsoft Visual Studio\2022\Professional',
        'C:\Program Files\Microsoft Visual Studio\2022\Professional',
        'D:\Program Files\Microsoft Visual Studio\2022\Enterprise',
        'C:\Program Files\Microsoft Visual Studio\2022\Enterprise'
    )) {
        if (Test-Path $candidate) { $vsRoot = $candidate; break }
    }
}
Write-Host "Using VS root: $vsRoot"

# ---- scope: per-user AND per-machine install locations ----
# VSIXInstaller with /a writes to Common7\IDE\Extensions (per-machine),
# otherwise it writes to %LOCALAPPDATA%\...\17.0_*\Extensions (per-user).
# Both are scanned by VS at startup; previous revisions of this script
# only looked at the per-user path and reported an empty Section 1 when
# the install was actually fine.
$extensionRoots = @(
    @{ Scope = 'per-user';    Path = (Join-Path $base 'Extensions') },
    @{ Scope = 'per-machine'; Path = (Join-Path $vsRoot 'Common7\IDE\Extensions') }
)

Write-Host '=== 1) Currently installed Lenovo Tidy extension dirs ==='
$found = $false
foreach ($root in $extensionRoots) {
    if (-not (Test-Path $root.Path)) { continue }
    Get-ChildItem $root.Path -Directory -EA 0 | ForEach-Object {
        $mf = Join-Path $_.FullName 'extension.vsixmanifest'
        if (Test-Path $mf) {
            $content = Get-Content $mf -Raw
            if ($content -match 'Identity Id="(LenovoTidy[^"]+)".*Version="([^"]+)"') {
                $found = $true
                Write-Host ("  Scope: " + $root.Scope)
                Write-Host ("  Dir  : " + $_.FullName)
                Write-Host ("  ID   : " + $Matches[1] + "  Version: " + $Matches[2])
                $dll = Join-Path $_.FullName 'LenovoTidy.dll'
                if (Test-Path $dll) {
                    $size  = (Get-Item $dll).Length
                    $mtime = (Get-Item $dll).LastWriteTime
                    Write-Host ("  DLL  : size=$size  mtime=$mtime")
                    try {
                        $asm = [Reflection.Assembly]::LoadFile($dll)
                        $ref = $asm.GetReferencedAssemblies() | Where-Object { $_.Name -eq 'Microsoft.VisualStudio.LanguageServer.Client' }
                        if ($ref) {
                            Write-Host ("  References LanguageServer.Client v" + $ref.Version)
                        }
                    } catch {
                        Write-Host ("  Reflection load error: " + $_.Exception.Message)
                    }
                }
                Write-Host ''
            }
        }
    }
}
if (-not $found) {
    Write-Host '  (no LenovoTidy extension found in either per-user or per-machine Extensions root)'
    Write-Host ''
}

Write-Host '=== 2) MEF cache freshness ==='
Get-ChildItem (Join-Path $base 'ComponentModelCache') -File -EA 0 |
    Sort-Object LastWriteTime -Descending |
    Select-Object Name, LastWriteTime, Length | Format-Table -AutoSize

Write-Host '=== 3) Does MEF cache have new ContentType (LenovoTidyCpp)? ==='
$cacheFile = Join-Path $base 'ComponentModelCache\Microsoft.VisualStudio.Default.cache'
if (Test-Path $cacheFile) {
    $bytes = [IO.File]::ReadAllBytes($cacheFile)
    $text  = [Text.Encoding]::ASCII.GetString($bytes)
    Write-Host ("  Cache mtime: " + (Get-Item $cacheFile).LastWriteTime)
    foreach ($probe in @('LenovoTidyCpp','LenovoTidy.LenovoTidyClient','LenovoTidy.LenovoTidyContentDefinition')) {
        if ($text.Contains($probe)) { Write-Host ("  YES  $probe") }
        else                          { Write-Host ("  NO   $probe") }
    }
    # Specifically: which LSP Client version is referenced near LenovoTidyClient?
    $idx = $text.IndexOf('LenovoTidy.LenovoTidyClient')
    if ($idx -gt 0) {
        $window = $text.Substring([Math]::Max(0,$idx-200), [Math]::Min(800, $text.Length - [Math]::Max(0,$idx-200)))
        if ($window -match 'LanguageServer\.Client[^,]*Version=([0-9\.]+)') {
            Write-Host ("  LSP Client version near LenovoTidyClient in cache: " + $Matches[1])
        }
    }
}

Write-Host ''
Write-Host '=== 4) Latest ActivityLog status ==='
$instanceLeaf = Split-Path $base -Leaf  # e.g. "17.0_ec695be7"
foreach ($rs in @($instanceLeaf, ($instanceLeaf + 'Exp'))) {
    $logFile = Join-Path ($env:APPDATA + '\Microsoft\VisualStudio') (Join-Path $rs 'ActivityLog.xml')
    if (Test-Path $logFile) {
        $mtime = (Get-Item $logFile).LastWriteTime
        Write-Host ("  $rs : mtime=$mtime  ageMin=" + [int]((Get-Date) - $mtime).TotalMinutes)
    }
}

Write-Host ''
Write-Host '=== 5) Running devenv ==='
Get-Process devenv -EA 0 | Select-Object Id, StartTime | Format-Table -AutoSize

Write-Host ''
Write-Host '=== 6) Vsix file we are deploying right now ==='
$vsix = 'D:\LenovoDRJ_CLang\dist\LenovoTidy.vsix'
Write-Host ("  $vsix")
Write-Host ("  size  = " + (Get-Item $vsix).Length)
Write-Host ("  mtime = " + (Get-Item $vsix).LastWriteTime)
Add-Type -AssemblyName System.IO.Compression.FileSystem
$tmp = Join-Path $env:TEMP ('lenovo-vsix-check-' + [Guid]::NewGuid())
[System.IO.Compression.ZipFile]::ExtractToDirectory($vsix, $tmp)
$dll = Join-Path $tmp 'LenovoTidy.dll'
if (Test-Path $dll) {
    $asm = [Reflection.Assembly]::LoadFile($dll)
    $ref = $asm.GetReferencedAssemblies() | Where-Object { $_.Name -eq 'Microsoft.VisualStudio.LanguageServer.Client' }
    Write-Host ("  Vsix payload LenovoTidy.dll references LanguageServer.Client v" + $ref.Version)
}
