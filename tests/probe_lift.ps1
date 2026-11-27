[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
New-Item -ItemType Directory -Force -Path (Join-Path $projectRoot 'build/tests') | Out-Null
& python (Join-Path $projectRoot 'tools/generate_messages.py') --input (Join-Path $projectRoot 'msg') --output (Join-Path $projectRoot 'build/generated/msg')
if ($LASTEXITCODE -ne 0) { throw 'Message generation failed.' }
$env:CBOARD_LIFT_TEST_ROOT = $projectRoot
$command = @'
set -e
root="$(cygpath -u "$CBOARD_LIFT_TEST_ROOT")"
cd "$root"
g++ -std=c++14 -Wall -Wextra -Werror -pthread \
  -I"$root" -I"$root/build/generated" -I"$root/platform/dji_cboard/include" \
  tests/probe_lift_test.cpp robot/drivers/probe_lift/ProbeLift.cpp \
  robot/params/ParamManager.cpp robot/params/ParamDefinitions.cpp \
  robot/orb/Topics.cpp robot/os/Mutex.cpp -o build/tests/probe_lift_test.exe
./build/tests/probe_lift_test.exe
g++ -std=c++14 -Wall -Wextra -Werror -pthread \
  -I"$root" -I"$root/build/generated" -I"$root/platform/dji_cboard/include" \
  tests/thickness_control_test.cpp robot/modules/thickness_control/ThicknessControl.cpp \
  robot/modules/thickness_control/ProbeLiftBench.cpp \
  robot/drivers/probe_lift/ProbeLift.cpp robot/params/ParamManager.cpp robot/params/ParamDefinitions.cpp \
  robot/orb/Topics.cpp robot/os/Mutex.cpp robot/os/Semaphore.cpp robot/work_queue/WorkItem.cpp \
  robot/work_queue/ScheduledWorkItem.cpp robot/work_queue/WorkQueue.cpp \
  robot/work_queue/WorkQueueManager.cpp robot/work_queue/WorkQueueConfigurations.cpp \
  -o build/tests/thickness_control_test.exe
./build/tests/thickness_control_test.exe
g++ -std=c++14 -Wall -Wextra -Werror -I"$root" tests/cboard_link_protocol_test.cpp \
  -o build/tests/cboard_link_protocol_test.exe
./build/tests/cboard_link_protocol_test.exe
'@
& 'C:\msys64\usr\bin\bash.exe' --login -c $command
if ($LASTEXITCODE -ne 0) { throw 'Probe lift or thickness control host tests failed.' }
