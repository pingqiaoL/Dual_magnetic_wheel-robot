# 本脚本检查并执行第四步 SBUS、uORB 和 rc_update 主机侧验收。
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

$requiredFiles = @(
    'robot\drivers\rc\sbus.h',
    'robot\drivers\rc\sbus.cpp',
    'robot\orb\uORB.hpp',
    'robot\orb\Topics.hpp',
    'robot\orb\Topics.cpp',
    'msg\InputRc.msg',
    'msg\RcChannels.msg',
    'msg\ManualControl.msg',
    'msg\ManualControlSwitches.msg',
    'tools\generate_messages.py',
    'robot\modules\SbusInput.hpp',
    'robot\modules\SbusInput.cpp',
    'robot\modules\RcConfig.hpp',
    'robot\modules\RcConfig.cpp',
    'robot\modules\RcUpdate.hpp',
    'robot\modules\RcUpdate.cpp',
    'robot\modules\SbusInput.cpp',
    'robot\modules\RcUpdate.cpp'
)

$missing = @($requiredFiles | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $projectRoot $_) -PathType Leaf)
})
if ($missing.Count -gt 0) {
    throw "Missing Step 4 files: $($missing -join ', ')"
}

$obsoleteTopicDirectory = Join-Path $projectRoot 'robot\orb\topics'
if (Test-Path -LiteralPath $obsoleteTopicDirectory) {
    throw 'uORB message definitions must be stored in the top-level msg directory.'
}

$handwrittenHeaders = @(Get-ChildItem -LiteralPath (Join-Path $projectRoot 'msg') `
    -Filter '*.hpp' -File -ErrorAction SilentlyContinue)
if ($handwrittenHeaders.Count -gt 0) {
    throw 'The msg directory must contain .msg sources instead of handwritten C++ headers.'
}

$defconfig = Get-Content -LiteralPath (
    Join-Path $projectRoot 'platform\dji_cboard\configs\robot\defconfig') -Raw
foreach ($setting in @(
    'CONFIG_STM32_USART3=y',
    'CONFIG_USART3_BAUD=100000',
    'CONFIG_USART3_2STOP=1',
    'CONFIG_USART3_PARITY=2'
)) {
    if ($defconfig -notmatch [regex]::Escape($setting)) {
        throw "Missing SBUS serial setting: $setting"
    }
}

$rcS = Get-Content -LiteralPath (
    Join-Path $projectRoot 'startup\etc\init.d\rcS') -Raw
foreach ($command in @('rc_update start', 'sbus_input start -d /dev/ttyS2')) {
    if ($rcS -notmatch [regex]::Escape($command)) {
        throw "rcS is missing command: $command"
    }
}

$msysBash = 'C:\msys64\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $msysBash -PathType Leaf)) {
    throw "MSYS2 bash was not found: $msysBash"
}

$testDirectory = Join-Path $projectRoot 'build\tests'
New-Item -ItemType Directory -Force -Path $testDirectory | Out-Null
$generatedDirectory = Join-Path $projectRoot 'build\generated\msg'
& python (Join-Path $projectRoot 'tools\generate_messages.py') `
    --input (Join-Path $projectRoot 'msg') --output $generatedDirectory
if ($LASTEXITCODE -ne 0) {
    throw 'uORB message generation failed.'
}
foreach ($header in @('InputRc.hpp', 'RcChannels.hpp', 'ManualControl.hpp',
        'ManualControlSwitches.hpp')) {
    if (-not (Test-Path -LiteralPath (Join-Path $generatedDirectory $header) -PathType Leaf)) {
        throw "Generated message header is missing: $header"
    }
}
$env:CBOARD_RC_TEST_ROOT = $projectRoot
$testCommand = @'
set -e
root="$(cygpath -u "$CBOARD_RC_TEST_ROOT")"
cd "$root"
g++ -std=c++14 -Wall -Wextra -Werror -I"$root/build/generated" -I"$root" \
  tests/sbus_decoder_test.cpp \
  robot/drivers/rc/sbus.cpp \
  robot/modules/RcConfig.cpp \
  robot/params/ParamManager.cpp \
  robot/orb/Topics.cpp robot/os/Mutex.cpp robot/os/Clock.cpp \
  -pthread \
  -o build/tests/sbus_decoder_test.exe
./build/tests/sbus_decoder_test.exe
'@

& $msysBash --login -c $testCommand
if ($LASTEXITCODE -ne 0) {
    throw 'SBUS decoder host compile or execution test failed.'
}

Write-Host '[PASS] Step 4 SBUS input, uORB topics, calibration, mapping, and RC update are complete.'
