# Wrapper invoked from WSL: launches install-prereqs.ps1 with admin elevation.
# Triggers exactly ONE UAC prompt (the user clicks Yes once).
# Waits for the elevated child to finish so WSL caller can read the log.

$ErrorActionPreference = 'Stop'

$here   = $PSScriptRoot
$target = Join-Path $here 'install-prereqs.ps1'
$marker = Join-Path $here 'logs\phase1-prereqs.done'
if (Test-Path $marker) { Remove-Item $marker -Force }

$argList = @(
    '-NoProfile',
    '-ExecutionPolicy', 'Bypass',
    '-Command',
    "& { try { & '$target'; \$ec = \$LASTEXITCODE } catch { \$ec = 1; \`$_ | Out-String | Set-Content '$here\logs\phase1-prereqs.error' } finally { Set-Content '$marker' \$ec } }"
)

Write-Host "Requesting elevation (UAC prompt) ..."
$proc = Start-Process -FilePath 'powershell.exe' -ArgumentList $argList -Verb RunAs -PassThru
$proc.WaitForExit()

if (-not (Test-Path $marker)) {
    Write-Error 'phase1-prereqs.done marker was not created (UAC denied?).'
    exit 1
}
$ec = (Get-Content $marker -Raw).Trim()
Write-Host "phase1-prereqs exit code: $ec"
exit ([int]$ec)
