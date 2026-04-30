# Common helpers shared by all phase scripts.
# Each phase script dot-sources this file:    . $PSScriptRoot\helpers.ps1
# All output is plain ASCII to avoid PS 5.1 encoding issues.

$ErrorActionPreference = 'Stop'
$ProgressPreference    = 'SilentlyContinue'
[Net.ServicePointManager]::SecurityProtocol = [Net.SecurityProtocolType]::Tls12

$script:Root         = 'D:\LenovoDRJ_CLang'
$script:WindowsBuild = Join-Path $script:Root 'windows-build'
$script:Downloads    = Join-Path $script:WindowsBuild 'downloads'
$script:LogsDir      = Join-Path $script:WindowsBuild 'logs'
$script:DevTools     = 'D:\dev-tools'
$script:DistDir      = Join-Path $script:Root 'dist'
$script:DistBin      = Join-Path $script:DistDir 'win32-x64'

foreach ($d in @($script:Downloads, $script:LogsDir, $script:DistDir, $script:DistBin, $script:DevTools)) {
    New-Item -ItemType Directory -Path $d -Force | Out-Null
}

$script:CurrentLogFile = $null

function Write-Log {
    param([string]$Message, [string]$Level = 'INFO')
    $line = "[{0}] [{1}] {2}" -f (Get-Date -Format 'HH:mm:ss'), $Level, $Message
    Write-Host $line
    if ($script:CurrentLogFile) {
        Add-Content -Path $script:CurrentLogFile -Value $line -Encoding UTF8
    }
}

function Start-PhaseLog {
    param([Parameter(Mandatory)] [string]$Name)
    $script:CurrentLogFile = Join-Path $script:LogsDir "$Name.log"
    if (Test-Path $script:CurrentLogFile) { Remove-Item $script:CurrentLogFile -Force }
    New-Item -ItemType File -Path $script:CurrentLogFile -Force | Out-Null
    Write-Log "=== Phase START: $Name ==="
    Write-Log "Host: $env:COMPUTERNAME  User: $env:USERNAME  PWSH: $($PSVersionTable.PSVersion)"
}

function Stop-PhaseLog {
    param([Parameter(Mandatory)] [string]$Name, [int]$ExitCode = 0)
    if ($ExitCode -eq 0) {
        Write-Log "=== Phase OK: $Name ==="
    } else {
        Write-Log "=== Phase FAILED ($ExitCode): $Name ===" 'ERROR'
    }
    $script:CurrentLogFile = $null
}

function Invoke-Download {
    param(
        [Parameter(Mandatory)] [string]$Url,
        [Parameter(Mandatory)] [string]$Destination,
        [int]$MaxAttempts = 3
    )
    if (Test-Path $Destination) {
        Write-Log "Cached download: $Destination"
        return
    }
    $tmp = "$Destination.partial"
    for ($i = 1; $i -le $MaxAttempts; $i++) {
        try {
            Write-Log "GET (try $i/$MaxAttempts): $Url"
            Invoke-WebRequest -Uri $Url -OutFile $tmp -UseBasicParsing -TimeoutSec 1800
            Move-Item $tmp $Destination -Force
            $size = (Get-Item $Destination).Length
            Write-Log ("Downloaded {0:N0} bytes -> {1}" -f $size, $Destination)
            return
        } catch {
            Write-Log "Download error: $($_.Exception.Message)" 'WARN'
            if (Test-Path $tmp) { Remove-Item $tmp -Force }
            if ($i -eq $MaxAttempts) { throw }
            Start-Sleep -Seconds 5
        }
    }
}

function Expand-ZipTo {
    param(
        [Parameter(Mandatory)] [string]$Zip,
        [Parameter(Mandatory)] [string]$Destination
    )
    if (Test-Path $Destination) {
        Write-Log "Removing existing: $Destination"
        Remove-Item -Recurse -Force $Destination
    }
    New-Item -ItemType Directory -Path $Destination -Force | Out-Null
    Write-Log "Expand $Zip -> $Destination"
    Expand-Archive -Path $Zip -DestinationPath $Destination -Force
}

function Test-Admin {
    $id = [Security.Principal.WindowsIdentity]::GetCurrent()
    $p  = New-Object Security.Principal.WindowsPrincipal $id
    return $p.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Add-PathPrefix {
    param([Parameter(Mandatory)] [string[]]$Paths)
    foreach ($p in $Paths) {
        if (-not $p) { continue }
        if (-not (Test-Path $p)) { continue }
        $sep = ';'
        $parts = $env:Path -split $sep
        if ($parts -notcontains $p) {
            $env:Path = "$p$sep$env:Path"
        }
    }
}

function Initialize-MsvcEnv {
    param([Parameter(Mandatory)] [string]$VsBuildToolsRoot)
    $vcvars = Join-Path $VsBuildToolsRoot 'VC\Auxiliary\Build\vcvars64.bat'
    if (-not (Test-Path $vcvars)) {
        throw "vcvars64.bat not found: $vcvars"
    }
    Write-Log "Loading MSVC env from $vcvars"
    $tmpFile = [IO.Path]::GetTempFileName()
    cmd.exe /c "`"$vcvars`" >NUL 2>&1 && set > `"$tmpFile`""
    $loaded = 0
    Get-Content $tmpFile | ForEach-Object {
        if ($_ -match '^([^=]+)=(.*)$') {
            $name  = $Matches[1]
            $value = $Matches[2]
            if ($name -notin @('PROCESSOR_ARCHITECTURE','PSModulePath')) {
                Set-Item -Path "Env:$name" -Value $value
                $loaded++
            }
        }
    }
    Remove-Item $tmpFile -Force
    Write-Log "Loaded $loaded MSVC env vars (cl.exe=$((Get-Command cl.exe -ErrorAction SilentlyContinue).Source))"
}

function Use-RustEnv {
    $rustHome = Join-Path $script:DevTools 'rust'
    $env:RUSTUP_HOME = Join-Path $rustHome 'rustup'
    $env:CARGO_HOME  = Join-Path $rustHome 'cargo'
    Add-PathPrefix @((Join-Path $env:CARGO_HOME 'bin'))
}

# Wrap a native-command invocation so PS does not throw on stderr.
# Returns $LASTEXITCODE; logs each line via Write-Log.
function Invoke-Native {
    param(
        [Parameter(Mandatory)] [scriptblock]$ScriptBlock
    )
    $prev = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        & $ScriptBlock 2>&1 | ForEach-Object {
            if ($_ -is [System.Management.Automation.ErrorRecord]) {
                Write-Log ("{0}" -f $_.Exception.Message)
            } else {
                Write-Log ("{0}" -f $_)
            }
        }
        return $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $prev
    }
}

function Use-ToolchainEnv {
    Add-PathPrefix @(
        (Join-Path $script:DevTools 'cmake\bin'),
        (Join-Path $script:DevTools 'ninja'),
        (Join-Path $script:DevTools 'python'),
        (Join-Path $script:DevTools 'python\Scripts'),
        (Join-Path $script:DevTools 'llvm18\bin')
    )
    $env:LLVM_DIR  = Join-Path $script:DevTools 'llvm18\lib\cmake\llvm'
    $env:Clang_DIR = Join-Path $script:DevTools 'llvm18\lib\cmake\clang'
}
