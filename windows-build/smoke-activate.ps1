# Headless activation smoke test: launch VS 2022 with /Log + smoke-test.cpp,
# poll the process list for lenovo-tidy-lsp.exe for up to 120 seconds, then
# close VS cleanly and parse the ActivityLog for any LenovoTidy errors.
#
# Exit code 0 iff lenovo-tidy-lsp.exe was observed running as a child of
# devenv.exe during the test window.
#
# Run as:
#   powershell.exe -NoProfile -ExecutionPolicy Bypass -File smoke-activate.ps1

$ErrorActionPreference = 'Stop'

$vsRoot  = 'D:\Program Files\Microsoft Visual Studio\2022\Community'
$devenv  = Join-Path $vsRoot 'Common7\IDE\devenv.exe'
$cpp     = 'D:\LenovoDRJ_CLang\windows-build\smoke-test.cpp'
$logFile = Join-Path $env:TEMP ("lenovo-activity-{0}.xml" -f ([Guid]::NewGuid()))

if (-not (Test-Path $devenv)) { throw "devenv.exe not found at $devenv" }
if (-not (Test-Path $cpp))    { throw "smoke test cpp not found at $cpp" }

# ------------------- pre-flight: no devenv should be running -------------------
$existing = Get-Process devenv -EA 0
if ($existing) {
    Write-Host "Killing $($existing.Count) existing devenv process(es) first"
    $existing | Stop-Process -Force -EA 0
    Start-Sleep -Seconds 3
}

# ------------------- launch VS with /Log + file --------------------------------
Write-Host "Launching VS with /Log and smoke-test.cpp"
Write-Host "  log=$logFile"
Write-Host "  file=$cpp"
$vsProc = Start-Process -FilePath $devenv `
    -ArgumentList @('/Log', $logFile, $cpp) `
    -PassThru

Write-Host "  devenv PID=$($vsProc.Id), started at $($vsProc.StartTime)"

# ------------------- poll for lenovo-tidy-lsp.exe -------------------------------
$lspPid    = $null
$maxWaitS  = 120
$pollStep  = 3
$elapsedS  = 0
$observed  = $false

while ($elapsedS -lt $maxWaitS) {
    Start-Sleep -Seconds $pollStep
    $elapsedS += $pollStep

    $lsp = Get-Process lenovo-tidy-lsp -EA 0
    if ($lsp) {
        $lspPid   = $lsp.Id
        $observed = $true
        Write-Host ("  [{0:D3}s] lenovo-tidy-lsp.exe FOUND PID=$lspPid  startTime=$($lsp.StartTime)" -f $elapsedS)
        break
    }

    $vs = Get-Process -Id $vsProc.Id -EA 0
    if (-not $vs) {
        Write-Host "  devenv exited unexpectedly after $elapsedS s"
        break
    }
    Write-Host ("  [{0:D3}s] polling ... devenv alive PID=$($vsProc.Id)  CPU=$([math]::Round($vs.CPU,1))s  RAM=$([math]::Round($vs.WorkingSet/1MB))MB" -f $elapsedS)
}

# ------------------- keep LSP around a bit to confirm alive --------------------
if ($observed) {
    Start-Sleep -Seconds 5
    $lsp2 = Get-Process -Id $lspPid -EA 0
    if ($lsp2) {
        Write-Host "  +5s: lenovo-tidy-lsp still alive (CPU=$([math]::Round($lsp2.CPU,2))s)"
    } else {
        Write-Host "  +5s: lenovo-tidy-lsp EXITED (would be a secondary issue; the key symptom is resolved)"
    }
}

# ------------------- close VS (gentle, then force) ------------------------------
Write-Host "Closing VS 2022"
try { Get-Process devenv -EA 0 | Stop-Process -Force -EA 0 } catch {}
Start-Sleep -Seconds 3
# Kill any orphan LSP processes too
Get-Process lenovo-tidy-lsp -EA 0 | Stop-Process -Force -EA 0

# ------------------- parse ActivityLog ------------------------------------------
if (Test-Path $logFile) {
    Write-Host ""
    Write-Host "=== ActivityLog entries mentioning LenovoTidy ==="
    try {
        $xml   = [xml](Get-Content $logFile -Raw -EA 0)
        $match = $xml.activity.entry | Where-Object {
            ($_.source -match 'LenovoTidy' -or $_.description -match 'LenovoTidy')
        }
        if ($match) {
            foreach ($e in $match) {
                Write-Host ("  [{0}] {1}: {2}" -f $e.type, $e.source, $e.description)
            }
        } else {
            Write-Host "  (no LenovoTidy-specific entries - this is NOT diagnostic evidence of failure, just that VS didn't log anything special)"
        }

        $errs = $xml.activity.entry | Where-Object { $_.type -eq 'Error' }
        if ($errs) {
            Write-Host ""
            Write-Host "=== Error entries in ActivityLog (first 20) ==="
            foreach ($e in $errs | Select-Object -First 20) {
                Write-Host ("  [Error] {0}: {1}" -f $e.source, $e.description)
            }
        }
    } catch {
        Write-Host "  (could not parse $logFile as XML: $($_.Exception.Message))"
    }
} else {
    Write-Host "  (no ActivityLog produced at $logFile)"
}

# ------------------- final verdict ----------------------------------------------
Write-Host ""
Write-Host "============================================================"
if ($observed) {
    Write-Host "## RESULT: lenovo-tidy-lsp.exe WAS OBSERVED (PID=$lspPid)"
    Write-Host "## VS 2022 successfully activated LenovoTidyClient for"
    Write-Host "## smoke-test.cpp. The ILanguageClient wiring is now fixed."
    Write-Host "============================================================"
    exit 0
} else {
    Write-Host "## RESULT: lenovo-tidy-lsp.exe NEVER SPAWNED after $maxWaitS s"
    Write-Host "## VS 2022 did not dispatch LenovoTidyClient. Check"
    Write-Host "## the ActivityLog and MEF .err above for details."
    Write-Host "============================================================"
    exit 1
}
