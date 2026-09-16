[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$requiredFiles = @(
    'README.md',
    'msg\README.md',
    'config\versions.env',
    'config\hardware.env',
    'docs\development-baseline.md',
    'docs\hardware-resource-map.md',
    'docs\source-layout.md',
    'tools\bootstrap.ps1',
    'tools\bootstrap.sh',
    'tools\build.ps1',
    'tools\build.sh',
    'tools\generate_messages.py',
    'tools\check_env.ps1',
    'tools\check_env.sh'
)

$missing = @()
foreach ($relative in $requiredFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $projectRoot $relative))) {
        $missing += $relative
    }
}

if ($missing.Count -gt 0) {
    Write-Error "Missing Step 1 files: $($missing -join ', ')"
    exit 1
}

$versions = Get-Content -LiteralPath (Join-Path $projectRoot 'config\versions.env') -Raw
if ($versions -notmatch 'NUTTX_TAG=nuttx-13\.0\.0' -or
    $versions -notmatch 'NUTTX_APPS_TAG=nuttx-13\.0\.0' -or
    $versions -notmatch 'NUTTX_ARCHIVE_SHA512=[0-9a-f]{128}' -or
    $versions -notmatch 'NUTTX_APPS_ARCHIVE_SHA512=[0-9a-f]{128}' -or
    $versions -notmatch 'NUTTX_COMMIT=273c77128b6698f0c95f0d7cde1d0bb803782021' -or
    $versions -notmatch 'NUTTX_APPS_COMMIT=20ffb1a3a3b590d52890ee865a28442390e5d16c') {
    Write-Error 'NuttX and NuttX Apps are not pinned to the expected matching release.'
    exit 1
}

$hardware = Get-Content -LiteralPath (Join-Path $projectRoot 'config\hardware.env') -Raw
foreach ($required in @('MCU=STM32F407IGH6', 'HSE_HZ=12000000', 'SBUS_UART=USART3', 'PRIMARY_MOTOR_CAN=CAN1')) {
    if ($hardware -notmatch [regex]::Escape($required)) {
        Write-Error "Missing hardware baseline: $required"
        exit 1
    }
}

Write-Host '[PASS] Step 1 project layout and pinned configuration are complete.'
