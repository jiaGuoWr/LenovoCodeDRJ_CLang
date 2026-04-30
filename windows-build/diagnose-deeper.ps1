$ErrorActionPreference = 'Continue'

# Auto-discover the VS 17.0 instance directory; honour LENOVO_VS_INSTANCE
# override. See diagnose-vs2022.ps1 for rationale.
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

Write-Host '=== A) MEF .err log lenovo-related sections ==='
$errLog = Join-Path $base 'ComponentModelCache\Microsoft.VisualStudio.Default.err'
$lines = Get-Content $errLog -Encoding UTF8
for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match 'LenovoTidy|lenovo-tidy') {
        # Print this line with 5 lines context
        $start = [Math]::Max(0, $i - 3)
        $end   = [Math]::Min($lines.Count - 1, $i + 5)
        Write-Host ('---- around line ' + $i + ' ----')
        for ($j = $start; $j -le $end; $j++) {
            $marker = if ($j -eq $i) { '>>' } else { '  ' }
            Write-Host ($marker + ' L' + $j + ': ' + $lines[$j])
        }
        Write-Host ''
        $i = $end  # skip past, don't double-print
    }
}

Write-Host ''
Write-Host '=== B) MEF cache - extract Export metadata for LenovoTidyClient ==='
# Cache is binary but strings inline. Look for the LenovoTidyClient block and
# print 200 chars after it - this contains the ContentType metadata MEF parsed.
$cacheFile = Join-Path $base 'ComponentModelCache\Microsoft.VisualStudio.Default.cache'
$bytes = [IO.File]::ReadAllBytes($cacheFile)
$text  = [Text.Encoding]::ASCII.GetString($bytes)
$marker = 'LenovoTidy.LenovoTidyClient'
$idx = $text.IndexOf($marker)
while ($idx -ge 0) {
    $end = [Math]::Min($text.Length, $idx + 600)
    $window = $text.Substring($idx, $end - $idx)
    # Print only printable ASCII characters
    $clean = -join ($window.ToCharArray() | ForEach-Object {
        if ([int]$_ -lt 32 -or [int]$_ -gt 126) { '.' } else { $_ }
    })
    Write-Host ('---- Hit at offset ' + $idx + ' ----')
    Write-Host $clean
    Write-Host ''
    $idx = $text.IndexOf($marker, $idx + 1)
}

Write-Host ''
Write-Host '=== C) ContentType registrations for .cpp / .h files ==='
$marker2 = '.cpp'
$idx = $text.IndexOf($marker2)
$count = 0
while ($idx -ge 0 -and $count -lt 5) {
    $start = [Math]::Max(0, $idx - 100)
    $end   = [Math]::Min($text.Length, $idx + 200)
    $window = $text.Substring($start, $end - $start)
    $clean = -join ($window.ToCharArray() | ForEach-Object {
        if ([int]$_ -lt 32 -or [int]$_ -gt 126) { '.' } else { $_ }
    })
    Write-Host ('---- .cpp hit at offset ' + $idx + ' ----')
    Write-Host $clean
    Write-Host ''
    $count++
    $idx = $text.IndexOf($marker2, $idx + 1)
}
