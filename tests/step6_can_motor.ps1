# 本脚本生成 uORB 头文件并执行达妙协议和 Climbot 分配主机测试。
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$generated = Join-Path $projectRoot 'build\generated\msg'
$testDirectory = Join-Path $projectRoot 'build\tests'
New-Item -ItemType Directory -Force -Path $testDirectory | Out-Null

$rcS = Get-Content -LiteralPath (
    Join-Path $projectRoot 'startup\etc\init.d\rcS') -Raw
if ($rcS -notmatch [regex]::Escape('source /etc/robots/climbot.sh')) {
    throw 'rcS must source the robot configuration in the current NSH.'
}
if ($rcS -notmatch [regex]::Escape('robot ready')) {
    throw 'rcS must mark module initialization complete.'
}

$configurationScript = Get-Content -LiteralPath (
    Join-Path $projectRoot 'startup\etc\robots\climbot.sh') -Raw
foreach ($command in @('control_allocator start -c climbot',
        'can_output start -d /dev/can0')) {
    if ($configurationScript -notmatch [regex]::Escape($command)) {
        throw "Climbot startup is missing command: $command"
    }
}

# NSH defaults to a short line buffer.  Check UTF-8 byte length because a
# truncated comment continues on the next read and is then parsed as a command.
$configurationPath = Join-Path $projectRoot 'startup\etc\robots\climbot.sh'
$lineNumber = 0
Get-Content -LiteralPath $configurationPath | ForEach-Object {
    ++$lineNumber
    $byteCount = [Text.Encoding]::UTF8.GetByteCount($_)
    if ($byteCount -gt 72) {
        throw "Climbot startup line $lineNumber is $byteCount UTF-8 bytes; maximum is 72."
    }
}

& python (Join-Path $projectRoot 'tools\generate_messages.py') `
    --input (Join-Path $projectRoot 'msg') --output $generated
if ($LASTEXITCODE -ne 0) { throw 'uORB message generation failed.' }

foreach ($header in @('ActuatorMotors.hpp', 'ActuatorServos.hpp',
        'ActuatorStatus.hpp')) {
    if (-not (Test-Path -LiteralPath (Join-Path $generated $header))) {
        throw "Generated message is missing: $header"
    }
}

$msysBash = 'C:\msys64\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $msysBash)) {
    throw "MSYS2 bash was not found: $msysBash"
}

$env:CBOARD_CAN_TEST_ROOT = $projectRoot
$command = @'
set -e
root="$(cygpath -u "$CBOARD_CAN_TEST_ROOT")"
cd "$root"
g++ -std=c++14 -Wall -Wextra -Werror -I"$root/build/generated" -I"$root" \
  tests/damiao_protocol_test.cpp protocol/can/DamiaoProtocol.cpp \
  -o build/tests/damiao_protocol_test.exe
./build/tests/damiao_protocol_test.exe

g++ -std=c++14 -Wall -Wextra -Werror -I"$root/build/generated" -I"$root" \
  tests/climbot_allocation_test.cpp robot/control/ClimbotAllocation.cpp \
  robot/params/ParamManager.cpp robot/orb/Topics.cpp \
  robot/os/Mutex.cpp robot/os/Clock.cpp -pthread \
  -o build/tests/climbot_allocation_test.exe
./build/tests/climbot_allocation_test.exe
'@

& $msysBash --login -c $command
if ($LASTEXITCODE -ne 0) {
    throw 'Step 6 host compile or execution test failed.'
}

Write-Host '[PASS] Step 6 Damiao protocol and Climbot allocation are complete.'
