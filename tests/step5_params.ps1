# 本脚本检查第五步参数系统布局，并运行主机侧参数管理器测试。
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

$requiredFiles = @(
    'msg\ParameterUpdate.msg',
    'robot\params\ParamTypes.hpp',
    'robot\params\ParamStorage.hpp',
    'robot\params\ParamManager.hpp',
    'robot\params\ParamManager.cpp',
    'robot\params\FlashParamStorage.hpp',
    'robot\params\FlashParamStorage.cpp',
    'robot\params\ParamSystem.hpp',
    'robot\params\ParamSystem.cpp',
    'apps\cboard\param_main.cpp'
)

$missing = @($requiredFiles | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $projectRoot $_) -PathType Leaf)
})
if ($missing.Count -gt 0) {
    throw "Missing Step 5 files: $($missing -join ', ')"
}

$linker = Get-Content -LiteralPath (
    Join-Path $projectRoot 'platform\dji_cboard\scripts\ld.script') -Raw
if ($linker -notmatch 'LENGTH\s*=\s*768K') {
    throw 'Firmware Flash must end before reserved parameter sectors 10 and 11.'
}

$board = Get-Content -LiteralPath (
    Join-Path $projectRoot 'platform\dji_cboard\include\board.h') -Raw
foreach ($address in @('0x080c0000', '0x080e0000')) {
    if ($board -notmatch $address) {
        throw "Missing parameter Flash slot: $address"
    }
}

$generatedDirectory = Join-Path $projectRoot 'build\generated\msg'
& python (Join-Path $projectRoot 'tools\generate_messages.py') `
    --input (Join-Path $projectRoot 'msg') --output $generatedDirectory
if ($LASTEXITCODE -ne 0) {
    throw 'uORB message generation failed.'
}

$msysBash = 'C:\msys64\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $msysBash -PathType Leaf)) {
    throw "MSYS2 bash was not found: $msysBash"
}

$testDirectory = Join-Path $projectRoot 'build\tests'
New-Item -ItemType Directory -Force -Path $testDirectory | Out-Null
$env:CBOARD_PARAM_TEST_ROOT = $projectRoot
$testCommand = @'
set -e
root="$(cygpath -u "$CBOARD_PARAM_TEST_ROOT")"
cd "$root"
g++ -std=c++14 -Wall -Wextra -Werror -pthread \
  -I"$root/build/generated" -I"$root" \
  tests/param_manager_test.cpp \
  robot/params/ParamManager.cpp robot/orb/Topics.cpp \
  robot/os/Mutex.cpp robot/os/Clock.cpp \
  -o build/tests/param_manager_test.exe
./build/tests/param_manager_test.exe
'@

& $msysBash --login -c $testCommand
if ($LASTEXITCODE -ne 0) {
    throw 'Parameter manager host compile or execution test failed.'
}

Write-Host '[PASS] Step 5 parameter runtime, uORB notification and Flash layout are complete.'
