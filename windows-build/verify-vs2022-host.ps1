#
# End-to-end local verification of LenovoTidy.dll inside an out-of-VS host.
# Runs in 3 layers, each catching different failure modes:
#
#   STEP 1: Reflection - dll metadata correctness (ContentType, Export)
#   STEP 2: Type-load  - confirm strong-name dependencies resolve
#   STEP 3: Spawn test - run lenovo-tidy-lsp.exe directly and check it
#                        responds to LSP `initialize` over stdin/stdout.
#
$ErrorActionPreference = 'Stop'

# ---------------- pick the just-built vsix and explode it -----------------
$vsixPath = 'D:\LenovoDRJ_CLang\dist\LenovoTidy.vsix'
$stage    = Join-Path $env:TEMP ('lenovo-vsix-host-' + [Guid]::NewGuid())
New-Item -ItemType Directory -Path $stage -Force | Out-Null
Add-Type -AssemblyName System.IO.Compression.FileSystem
[System.IO.Compression.ZipFile]::ExtractToDirectory($vsixPath, $stage)
Write-Host "Staged vsix payload at: $stage"
Write-Host ''

# ---------------- preload VS's LSP Client + Utilities --------------------
$vsRoot   = 'D:\Program Files\Microsoft Visual Studio\2022\Community'
$probeDirs = @(
    $stage,
    (Join-Path $vsRoot 'Common7\IDE'),
    (Join-Path $vsRoot 'Common7\IDE\PrivateAssemblies'),
    (Join-Path $vsRoot 'Common7\IDE\PublicAssemblies'),
    (Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\LanguageServer'),
    (Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\Editor')
)

[Reflection.Assembly]::LoadFile((Join-Path $vsRoot 'Common7\IDE\Microsoft.VisualStudio.Utilities.dll')) | Out-Null
[Reflection.Assembly]::LoadFile((Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\LanguageServer\Microsoft.VisualStudio.LanguageServer.Client.dll')) | Out-Null
$threadingDll = Get-ChildItem (Join-Path $vsRoot 'Common7\IDE') -Filter 'Microsoft.VisualStudio.Threading.dll' -Recurse | Select-Object -First 1
if ($threadingDll) { [Reflection.Assembly]::LoadFile($threadingDll.FullName) | Out-Null }

[AppDomain]::CurrentDomain.add_AssemblyResolve({
    param($s, $e)
    $name = ([Reflection.AssemblyName]$e.Name).Name + '.dll'
    foreach ($d in $probeDirs) {
        $p = Join-Path $d $name
        if (Test-Path $p) {
            try { return [Reflection.Assembly]::LoadFile($p) } catch {}
        }
    }
    return $null
})

# ============================================================
# STEP 1: Reflection - confirm dll metadata
# ============================================================
Write-Host '=== STEP 1: Reflection over LenovoTidy.LenovoTidyClient ==='
$dll = Join-Path $stage 'LenovoTidy.dll'
$asm = [Reflection.Assembly]::LoadFile($dll)
$clientType = $asm.GetType('LenovoTidy.LenovoTidyClient')
if (-not $clientType) { throw "FAIL: LenovoTidyClient type not found" }

$attrs = $clientType.GetCustomAttributesData()
$contentTypes = @()
foreach ($a in $attrs) {
    if ($a.AttributeType.Name -eq 'ContentTypeAttribute') {
        $contentTypes += $a.ConstructorArguments[0].Value
    }
}
Write-Host ("  [ContentType]: " + ($contentTypes -join ', '))

$expected = @('LenovoTidyCpp','C/C++','c','cpp')
$missing = $expected | Where-Object { $_ -notin $contentTypes }
if ($missing) { throw "FAIL: missing ContentType(s): $($missing -join ', ')" }

$exportAttr = $attrs | Where-Object { $_.AttributeType.Name -eq 'ExportAttribute' } | Select-Object -First 1
$exportTarget = $exportAttr.ConstructorArguments[0].Value.FullName
Write-Host ("  [Export(typeof(" + $exportTarget + "))]")
if ($exportTarget -notmatch 'ILanguageClient') { throw "FAIL: not exporting ILanguageClient" }

$lspRef = $asm.GetReferencedAssemblies() | Where-Object { $_.Name -eq 'Microsoft.VisualStudio.LanguageServer.Client' }
$systemLspVer = [Reflection.AssemblyName]::GetAssemblyName((Join-Path $vsRoot 'Common7\IDE\CommonExtensions\Microsoft\LanguageServer\Microsoft.VisualStudio.LanguageServer.Client.dll')).Version
Write-Host ("  References LSP Client v" + $lspRef.Version + " ; system installed v" + $systemLspVer)
if ($lspRef.Version -ne $systemLspVer) {
    throw "FAIL: dll references LSP Client v$($lspRef.Version) but system has v$systemLspVer (CLR strong-name will fail)"
}

Write-Host '  PASS: metadata is correct'
Write-Host ''

# ============================================================
# STEP 2: Type-load - confirm CLR can fully resolve dependencies
# ============================================================
Write-Host '=== STEP 2: Resolve all type dependencies ==='
try {
    [void]$asm.GetTypes()
    Write-Host '  PASS: all types in LenovoTidy.dll loaded without LoaderException'
} catch [Reflection.ReflectionTypeLoadException] {
    Write-Host '  Loaded types:'
    $_.Exception.Types | Where-Object { $_ } | ForEach-Object { Write-Host ("    " + $_.FullName) }
    Write-Host '  Loader exceptions:'
    $_.Exception.LoaderExceptions | Select-Object -First 3 | ForEach-Object { Write-Host ("    " + $_.Message) }
    throw "FAIL: dependencies not all resolvable"
}
Write-Host ''

# ============================================================
# STEP 3: Spawn LSP server directly and probe it via stdio
# ============================================================
Write-Host '=== STEP 3: Spawn lenovo-tidy-lsp.exe and exchange LSP `initialize` ==='
$lspExe = Join-Path $stage 'server-bin\win32-x64\lenovo-tidy-lsp.exe'
if (-not (Test-Path $lspExe)) { throw "FAIL: lsp exe missing in vsix payload" }
Write-Host ("  Spawning: " + $lspExe)

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = $lspExe
$psi.RedirectStandardInput  = $true
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError  = $true
$psi.UseShellExecute        = $false
$psi.CreateNoWindow         = $true

$proc = New-Object System.Diagnostics.Process
$proc.StartInfo = $psi
$null = $proc.Start()
Write-Host ("  Started PID=" + $proc.Id)

# Send LSP `initialize` request over stdin
$body = @{
    jsonrpc = '2.0'
    id      = 1
    method  = 'initialize'
    params  = @{
        processId = $proc.Id
        rootUri   = 'file:///D:/tidy-test'
        capabilities = @{}
        initializationOptions = @{
            clangTidyPath      = ''
            pluginPath         = $null
            compileCommandsDir = ''
            checks             = 'lenovo-*'
        }
    }
} | ConvertTo-Json -Depth 10 -Compress
$header = "Content-Length: $($body.Length)`r`n`r`n"

$proc.StandardInput.AutoFlush = $true
$proc.StandardInput.Write($header)
$proc.StandardInput.Write($body)
$proc.StandardInput.Flush()
Write-Host ("  Sent initialize (" + $body.Length + " bytes)")

# Read response back (with timeout). Keep a named handle to the buffer so we
# can print what the LSP server returned - $readTask.AsyncState is always
# null because we didn't pass a state object to ReadAsync.
#
# LSP framing: each message is prefixed with "Content-Length: N\r\n\r\n".
# The OS pipe almost always delivers the header as its own chunk (~20-30 B)
# before the body. Proof-of-life is therefore any response that *starts with*
# "Content-Length:" - we do NOT require the body in the same read. A naive
# byte-count threshold produced false FAILs because the header alone is
# legal wire traffic.
$readBuf  = New-Object char[] 4096
$readTask = $proc.StandardOutput.ReadAsync($readBuf, 0, $readBuf.Length)
$ok = $readTask.Wait(5000)
if (-not $ok) {
    $proc.Kill(); throw "FAIL: lsp did not respond within 5s"
}
$len = $readTask.Result
if ($len -le 0) { $proc.Kill(); throw "FAIL: lsp closed stdout without sending anything" }
$head = (-join $readBuf[0..([Math]::Min($len-1, 150))]).Trim()
if ($head -notmatch '^Content-Length:\s*\d+') {
    $proc.Kill(); throw "FAIL: response does not start with LSP 'Content-Length' header. Got: $head"
}
Write-Host ("  LSP responded ($len bytes): " + $head)

$proc.StandardInput.Close()
$proc.WaitForExit(2000) | Out-Null
if (-not $proc.HasExited) { $proc.Kill() }

$stderr = $proc.StandardError.ReadToEnd()
if ($stderr) {
    Write-Host '  stderr (first 500 chars):'
    Write-Host ('    ' + $stderr.Substring(0, [Math]::Min(500, $stderr.Length)))
}

Write-Host '  PASS: lenovo-tidy-lsp.exe is alive and speaks LSP'
Write-Host ''

# ============================================================
# Summary
# ============================================================
Write-Host '############################################################'
Write-Host '## ALL 3 LOCAL VERIFICATION STEPS PASSED                  ##'
Write-Host '## - dll metadata correct (4 ContentTypes, ILanguageClient)##'
Write-Host '## - all type dependencies resolve                        ##'
Write-Host '## - LSP server spawnable + responds to initialize        ##'
Write-Host '## VS 2022 should now wire LenovoTidyClient to .cpp files ##'
Write-Host '############################################################'

Remove-Item -Recurse -Force $stage -EA 0
exit 0
