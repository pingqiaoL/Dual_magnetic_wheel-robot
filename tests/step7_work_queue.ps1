[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path (Join-Path $projectRoot 'build/tests') | Out-Null
& python (Join-Path $projectRoot 'tools/generate_messages.py') --input (Join-Path $projectRoot 'msg') --output (Join-Path $projectRoot 'build/generated/msg')
if ($LASTEXITCODE -ne 0) { throw 'Message generation failed.' }
$env:CBOARD_WQ_TEST_ROOT = $projectRoot
$command = @'
set -e
root="$(cygpath -u "$CBOARD_WQ_TEST_ROOT")"
cd "$root"
g++ -std=c++14 -Wall -Wextra -Werror -I"$root" \
  tests/work_queue_test.cpp robot/work_queue/WorkQueue.cpp \
  robot/os/Mutex.cpp robot/os/Clock.cpp -pthread \
  -o build/tests/work_queue_test.exe
./build/tests/work_queue_test.exe
g++ -std=c++14 -Wall -Wextra -Werror -I"$root/build/generated" -I"$root" \
  tests/work_queue_modules_test.cpp robot/modules/RcUpdate.cpp \
  robot/modules/RcConfig.cpp robot/modules/Command.cpp \
  robot/modules/Failsafe.cpp robot/modules/RobotControl.cpp \
  robot/modules/ControlAllocator.cpp robot/control/Allocation.cpp \
  robot/control/ActuatorEffectivenessDualMagneticWheel.cpp \
  robot/work_queue/WorkQueue.cpp robot/work_queue/WorkQueueManager.cpp \
  robot/params/ParamManager.cpp robot/params/ParamDefinitions.cpp \
  robot/params/ModuleParams.cpp robot/orb/Topics.cpp \
  robot/os/Mutex.cpp robot/os/Clock.cpp -pthread \
  -o build/tests/work_queue_modules_test.exe
./build/tests/work_queue_modules_test.exe
'@
& 'C:\msys64\usr\bin\bash.exe' --login -c $command
if ($LASTEXITCODE -ne 0) { throw 'Work queue host tests failed.' }
Write-Host '[PASS] Work queues, callbacks, module lifecycle and fallback safety.'
