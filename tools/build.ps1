[CmdletBinding()]
param(
    [switch]$Clean,
    [int]$Jobs = 0
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

function Get-WslBuildEnvironment {
    $wsl = Get-Command 'wsl.exe' -ErrorAction SilentlyContinue
    if ($null -eq $wsl) {
        return $null
    }

    & $wsl.Source -e sh -lc 'command -v bash >/dev/null && command -v make >/dev/null && command -v arm-none-eabi-gcc >/dev/null' *> $null
    if ($LASTEXITCODE -eq 0) {
        return $wsl.Source
    }

    return $null
}

function Find-Stm32CubeTool {
    param([Parameter(Mandatory)][string]$Pattern)

    $command = Get-Command $Pattern -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    $roots = @(
        'C:\ST',
        'D:\ST',
        'C:\Program Files\STMicroelectronics',
        'D:\Program Files\STMicroelectronics'
    )

    $matches = foreach ($root in $roots) {
        if (Test-Path -LiteralPath $root) {
            Get-ChildItem -LiteralPath $root -Recurse -Filter $Pattern -File -ErrorAction SilentlyContinue
        }
    }

    return $matches | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
}

$buildArgs = @()
if ($Clean) {
    $buildArgs += '--clean'
}
if ($Jobs -gt 0) {
    $buildArgs += '--jobs'
    $buildArgs += $Jobs.ToString()
}

$wslPath = Get-WslBuildEnvironment
if ($null -ne $wslPath) {
    $linuxRoot = (& $wslPath wslpath -a $projectRoot).Trim()
    if ($LASTEXITCODE -ne 0 -or -not $linuxRoot) {
        throw 'The WSL project path could not be resolved.'
    }

    $quotedArgs = ($buildArgs | ForEach-Object { "'$_'" }) -join ' '
    & $wslPath -e bash -lc "cd '$linuxRoot' && ./tools/build.sh $quotedArgs"
    exit $LASTEXITCODE
}

$msysBash = 'C:\msys64\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $msysBash -PathType Leaf)) {
    throw 'No usable WSL toolchain or C:\msys64 environment was found. Run tools\setup_build_env.ps1.'
}

$armGcc = Find-Stm32CubeTool -Pattern 'arm-none-eabi-gcc.exe'
if (-not $armGcc) {
    throw 'Arm GCC was not found in PATH or an STM32CubeIDE installation.'
}

$msysJobs = if ($Jobs -gt 0) { $Jobs } else { 4 }
if ($msysJobs -gt 4) {
    Write-Host "MSYS2 build concurrency limited from $msysJobs to 4 to avoid process stalls."
    $msysJobs = 4
}

$env:CBOARD_PROJECT_ROOT = $projectRoot
$env:CBOARD_ARM_BIN = Split-Path -Parent $armGcc
$env:CBOARD_BUILD_CLEAN = if ($Clean) { '1' } else { '0' }
$env:CBOARD_BUILD_JOBS = $msysJobs.ToString()

$command = @'
set -euo pipefail
project_root="$(cygpath -u "$CBOARD_PROJECT_ROOT")"
arm_bin="$(cygpath -u "$CBOARD_ARM_BIN")"
export PATH="$arm_bin:$PATH"
args=(--host-msys)
if [[ "$CBOARD_BUILD_CLEAN" == "1" ]]; then args+=(--clean); fi
if [[ "$CBOARD_BUILD_JOBS" != "0" ]]; then args+=(--jobs "$CBOARD_BUILD_JOBS"); fi
cd "$project_root"
bash ./tools/build.sh "${args[@]}"
'@

$wrapperName = "cboard-build-$PID-$([Guid]::NewGuid().ToString('N')).sh"
$wrapperWindowsPath = Join-Path 'C:\msys64\tmp' $wrapperName
$wrapperMsysPath = "/tmp/$wrapperName"
$utf8WithoutBom = [System.Text.UTF8Encoding]::new($false)

try {
    [System.IO.File]::WriteAllText($wrapperWindowsPath, $command, $utf8WithoutBom)
    & $msysBash --login $wrapperMsysPath
    $buildExitCode = $LASTEXITCODE
} finally {
    Remove-Item -LiteralPath $wrapperWindowsPath -Force -ErrorAction SilentlyContinue
}

exit $buildExitCode
