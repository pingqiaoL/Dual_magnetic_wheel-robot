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
foreach ($command in @('source /etc/init.d/rc.autostart',
        'control_allocator start', 'can_output start -d /dev/can0',
        'command start', 'robot ready')) {
    if ($rcS -notmatch [regex]::Escape($command)) {
        throw "rcS is missing command: $command"
    }
}
$selector = Get-Content -LiteralPath (
    Join-Path $projectRoot 'startup\etc\init.d\rc.autostart') -Raw
foreach ($command in @('param compare SYS_AUTOSTART 1',
        'source /etc/robots/1_dual_magneticwheel')) {
    if ($selector -notmatch [regex]::Escape($command)) {
        throw "Profile selector is missing: $command"
    }
}
$profile = Get-Content -LiteralPath (
    Join-Path $projectRoot 'startup\etc\robots\1_dual_magneticwheel') -Raw
if ($profile -notmatch 'param set CA_AIRFRAME 1') {
    throw 'Numbered profile must select its actuator model.'
}
# NSH短行缓冲区按UTF-8字节检查，避免注释截断后当成命令执行。
Get-ChildItem -LiteralPath (Join-Path $projectRoot 'startup\etc') -Recurse -File |
    ForEach-Object {
        $script = $_
        $lineNumber = 0
        Get-Content -LiteralPath $script.FullName | ForEach-Object {
            ++$lineNumber
            $byteCount = [Text.Encoding]::UTF8.GetByteCount($_)
            if ($byteCount -gt 72) {
                throw "$($script.Name):$lineNumber has $byteCount bytes; max 72."
            }
        }
    }

& python (Join-Path $projectRoot 'tools\generate_messages.py') `
    --input (Join-Path $projectRoot 'msg') --output $generated
if ($LASTEXITCODE -ne 0) { throw 'uORB message generation failed.' }

foreach ($header in @('ActuatorMotors.hpp', 'ActuatorServos.hpp',
        'ActuatorStatus.hpp', 'ActuatorArmed.hpp')) {
    if (-not (Test-Path -LiteralPath (Join-Path $generated $header))) {
        throw "Generated message is missing: $header"
    }
}

$msysBash = 'C:\msys64\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $msysBash)) {
    throw "MSYS2 bash was not found: $msysBash"
}

& python (Join-Path $projectRoot 'tests\prepare_nsh_eof_test.py')
if ($LASTEXITCODE -ne 0) { throw 'NSH EOF regression fixture preparation failed.' }

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
  tests/climbot_allocation_test.cpp robot/control/Allocation.cpp \
  robot/control/ActuatorEffectivenessDualMagneticWheel.cpp \
  robot/params/ParamManager.cpp robot/orb/Topics.cpp \
  robot/os/Mutex.cpp robot/os/Clock.cpp -pthread \
  -o build/tests/climbot_allocation_test.exe
./build/tests/climbot_allocation_test.exe

g++ -std=c++14 -Wall -Wextra -Werror \
  -I"$root/build/generated" -I"$root" \
  tests/output_chain_test.cpp robot/modules/Command.cpp \
  robot/output/MixingOutput.cpp robot/control/Allocation.cpp \
  robot/control/ActuatorEffectivenessDualMagneticWheel.cpp \
  robot/params/ParamManager.cpp robot/orb/Topics.cpp \
  robot/os/Mutex.cpp robot/os/Clock.cpp -pthread \
  -o build/tests/output_chain_test.exe
./build/tests/output_chain_test.exe

g++ -std=c++14 -Wall -Wextra -Werror -I"$root/build/generated" -I"$root" \
  tests/param_command_test.cpp robot/params/param_main.cpp \
  robot/params/ParamManager.cpp robot/orb/Topics.cpp \
  robot/os/Mutex.cpp robot/os/Clock.cpp -pthread \
  -o build/tests/param_command_test.exe
./build/tests/param_command_test.exe

gcc -std=c11 -D_GNU_SOURCE -Wall -Wextra -Werror -I"$root/build/tests/nsh_eof" \
  tests/nsh_script_eof_test.c build/tests/nsh_eof/nshlib/nsh_script.c \
  -o build/tests/nsh_script_eof_test.exe
./build/tests/nsh_script_eof_test.exe
'@

& $msysBash --login -c $command
if ($LASTEXITCODE -ne 0) {
    throw 'Step 6 host compile or execution test failed.'
}

Write-Host '[PASS] Step 6 Damiao, matrix allocation, command safety and mixing output are complete.'
