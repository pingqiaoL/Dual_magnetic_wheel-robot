# 使用正点原子高速无线调试器的 CMSIS-DAP 接口烧录并校验 NuttX 固件。
[CmdletBinding()]
param(
    [string]$Firmware = 'artifacts\nuttx.hex',
    [ValidateRange(10, 1000)]
    [int]$AdapterKhz = 50
)

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$firmwarePath = if ([IO.Path]::IsPathRooted($Firmware)) {
    $Firmware
} else {
    Join-Path $projectRoot $Firmware
}

if (-not (Test-Path -LiteralPath $firmwarePath -PathType Leaf)) {
    throw "Firmware was not found: $firmwarePath"
}

$openOcd = Get-ChildItem -LiteralPath 'C:\ST' -Recurse `
    -Filter 'openocd.exe' -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match 'externaltools\.openocd' } |
    Sort-Object FullName -Descending |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $openOcd) {
    throw 'STM32CubeIDE OpenOCD was not found below C:\ST.'
}

$cmsisDap = Get-ChildItem -LiteralPath 'C:\ST' -Recurse `
    -Filter 'cmsis-dap.cfg' -File -ErrorAction SilentlyContinue |
    Where-Object { $_.FullName -match 'st_scripts[\\/]interface' } |
    Sort-Object FullName -Descending |
    Select-Object -First 1
if (-not $cmsisDap) {
    throw 'STM32CubeIDE CMSIS-DAP OpenOCD scripts were not found.'
}

$scriptsRoot = Split-Path -Parent (Split-Path -Parent $cmsisDap.FullName)
# Windows PowerShell 5.1 使用的旧版 .NET 没有 Path.GetRelativePath()。
# 固件位于工程目录内时手动截取相对路径，保证脚本同时兼容 5.1 和 7.x。
$resolvedProjectRoot = (Resolve-Path -LiteralPath $projectRoot).Path.TrimEnd('\')
$resolvedFirmware = (Resolve-Path -LiteralPath $firmwarePath).Path
$projectPrefix = $resolvedProjectRoot + '\'

if ($resolvedFirmware.StartsWith($projectPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    $openOcdFirmware = $resolvedFirmware.Substring($projectPrefix.Length)
} else {
    $openOcdFirmware = $resolvedFirmware
}

$openOcdFirmware = $openOcdFirmware.Replace('\', '/')
$adapterConfig = "transport select swd; set CLOCK_FREQ $AdapterKhz; " +
    'set CONNECT_UNDER_RESET 0; reset_config none'
$flashCommands = "init; halt; flash write_image erase {$openOcdFirmware}; " +
    "verify_image {$openOcdFirmware}; reset run; shutdown"

Push-Location $projectRoot
try {
    # OpenOCD 会把版本及进度信息写到 stderr。Windows PowerShell 5.1 会将这些
    # 信息包装成 NativeCommandError，因此执行原生命令期间临时允许非终止错误，
    # 最终仍通过退出码及 OpenOCD 的 Error 文本判断烧录是否真正失败。
    $savedErrorActionPreference = $ErrorActionPreference
    $output = New-Object 'System.Collections.Generic.List[string]'
    try {
        $ErrorActionPreference = 'Continue'
        & $openOcd -s $scriptsRoot -f 'interface/cmsis-dap.cfg' `
            -c $adapterConfig -f 'target/stm32f4x.cfg' -c $flashCommands 2>&1 |
            ForEach-Object {
                $line = $_.ToString()
                $output.Add($line)
                Write-Host $line
            }
        $exitCode = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $savedErrorActionPreference
    }

    $errorText = $output -join [Environment]::NewLine
    if ($exitCode -ne 0 -or $errorText -match '(?m)^Error:|Unable to') {
        throw "Wireless flash failed. OpenOCD exit code: $exitCode"
    }
} finally {
    Pop-Location
}

Write-Host "[PASS] Wireless flash, verification and reset completed: $firmwarePath"
