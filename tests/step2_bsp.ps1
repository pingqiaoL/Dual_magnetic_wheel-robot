[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$boardRoot = Join-Path $projectRoot 'platform\dji_cboard'

$requiredFiles = @(
    'CMakeLists.txt',
    'Kconfig',
    'configs\robot\defconfig',
    'include\board.h',
    'scripts\Make.defs',
    'scripts\ld.script',
    'src\CMakeLists.txt',
    'src\Make.defs',
    'src\dji_cboard.h',
    'src\stm32_boot.c',
    'src\stm32_bringup.c',
    'src\stm32_autoleds.c',
    'src\stm32_buttons.c'
)

$missing = @($requiredFiles | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $boardRoot $_) -PathType Leaf)
})
if ($missing.Count -gt 0) {
    throw "Missing Step 2 BSP files: $($missing -join ', ')"
}

$defconfig = Get-Content -LiteralPath (Join-Path $boardRoot 'configs\robot\defconfig') -Raw
foreach ($setting in @(
    'CONFIG_ARCH_BOARD_CUSTOM=y',
    'CONFIG_ARCH_CHIP_STM32F407IG=y',
    'CONFIG_USART6_SERIAL_CONSOLE=y',
    'CONFIG_STM32_USART3=y',
    'CONFIG_STM32_USART1=y',
    'CONFIG_USART3_BAUD=100000',
    'CONFIG_USART3_PARITY=2',
    'CONFIG_USART3_2STOP=1',
    'CONFIG_STM32_FLASH_CONFIG_G=y'
)) {
    if ($defconfig -notmatch [regex]::Escape($setting)) {
        throw "Missing board setting: $setting"
    }
}

$boardHeader = Get-Content -LiteralPath (Join-Path $boardRoot 'include\board.h') -Raw
foreach ($mapping in @(
    'STM32_BOARD_XTAL              12000000ul',
    'GPIO_USART3_RX_2',
    'GPIO_USART6_TX_2',
    'GPIO_CAN1_RX_3',
    'GPIO_TIM1_CH1OUT_2'
)) {
    if ($boardHeader -notmatch [regex]::Escape($mapping)) {
        throw "Missing verified pin/clock mapping: $mapping"
    }
}

$linker = Get-Content -LiteralPath (Join-Path $boardRoot 'scripts\ld.script') -Raw
if ($linker -notmatch 'ORIGIN = 0x08000000, LENGTH = 768K' -or
    $linker -notmatch 'ORIGIN = 0x20000000, LENGTH = 112K') {
    throw 'Linker memory map does not reserve the final 256 KiB for parameters.'
}

foreach ($mapping in @(
    'BOARD_PARAM_FLASH_SLOT_A      0x080c0000ul',
    'BOARD_PARAM_FLASH_SLOT_B      0x080e0000ul',
    'BOARD_PARAM_FLASH_SLOT_SIZE   (128ul * 1024ul)'
)) {
    if ($boardHeader -notmatch [regex]::Escape($mapping)) {
        throw "Missing parameter Flash mapping: $mapping"
    }
}

$projectSources = Get-ChildItem -LiteralPath $boardRoot -Recurse -File |
    Where-Object { $_.Extension -in @('.c', '.h', '.cpp', '.hpp') }
foreach ($source in $projectSources) {
    $sourceText = Get-Content -LiteralPath $source.FullName -Raw
    if ($sourceText -match '#include\s+[<"]robot/') {
        throw "BSP depends on robot business code: $($source.FullName)"
    }
}

Write-Host '[PASS] Step 2 C-board BSP structure, target, memory, and pin mappings are complete.'
