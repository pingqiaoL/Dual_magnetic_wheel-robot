# 本脚本检查第三步 CRTP 模块框架、OS 封装和启动脚本是否完整。
[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot

$requiredFiles = @(
    'robot\common\ModuleBase.hpp',
    'robot\os\Task.hpp',
    'robot\os\Task.cpp',
    'robot\os\Mutex.hpp',
    'robot\os\Mutex.cpp',
    'robot\os\Semaphore.hpp',
    'robot\os\Semaphore.cpp',
    'robot\os\Clock.hpp',
    'robot\os\Clock.cpp',
    'robot\modules\RobotRuntime.hpp',
    'robot\modules\RobotRuntime.cpp',
    'config\nuttx\Kconfig',
    'config\nuttx\Make.defs',
    'config\nuttx\Makefile',
    'robot\modules\RobotRuntime.cpp',
    'startup\etc\init.d\rcS',
    'startup\etc\init.d\rc.sysinit',
    'tools\generate_romfs.py'
)

$missing = @($requiredFiles | Where-Object {
    -not (Test-Path -LiteralPath (Join-Path $projectRoot $_) -PathType Leaf)
})
if ($missing.Count -gt 0) {
    throw "Missing Step 3 files: $($missing -join ', ')"
}

$defconfig = Get-Content -LiteralPath (
    Join-Path $projectRoot 'platform\dji_cboard\configs\robot\defconfig') -Raw
foreach ($setting in @(
    'CONFIG_CBOARD_ROBOT=y',
    'CONFIG_CBOARD_ROBOT_PROGNAME="robot"',
    'CONFIG_ETC_ROMFS=y',
    'CONFIG_FS_ROMFS=y'
)) {
    if ($defconfig -notmatch [regex]::Escape($setting)) {
        throw "Missing Step 3 setting: $setting"
    }
}

$robotMain = Get-Content -LiteralPath (
    Join-Path $projectRoot 'robot\modules\RobotRuntime.cpp') -Raw
if ($robotMain -notmatch 'RobotRuntime::main\(argc, argv\)') {
    throw 'robot command does not delegate to the CRTP ModuleBase entry.'
}

# 模块入口必须与实现同文件，且工程不再保留独立apps目录。
if (Test-Path -LiteralPath (Join-Path $projectRoot 'apps')) {
    throw 'Project-owned apps directory must be removed.'
}
$moduleEntries = @{
    'robot\modules\RobotRuntime.cpp' = 'robot_main'
    'robot\modules\SbusInput.cpp' = 'sbus_input_main'
    'robot\modules\RcUpdate.cpp' = 'rc_update_main'
    'robot\modules\ControlAllocator.cpp' = 'control_allocator_main'
    'robot\modules\Command.cpp' = 'command_main'
    'robot\output\CanOutput.cpp' = 'can_output_main'
}
foreach ($file in $moduleEntries.Keys) {
    $source = Get-Content -LiteralPath (Join-Path $projectRoot $file) -Raw
    $entry = [regex]::Escape($moduleEntries[$file])
    if ($source -notmatch ('extern "C" int ' + $entry +
            '\(int argc, char \*argv\[\]\)\s*\{[^}]+\}\s*$')) {
        throw "Module entry is missing from the end of: $file"
    }
}

$moduleBase = Get-Content -LiteralPath (
    Join-Path $projectRoot 'robot\common\ModuleBase.hpp') -Raw
foreach ($requiredPattern in @(
    'template<class T>',
    'T::task_spawn',
    'T::instantiate',
    'CommandRouter::dispatch',
    'T::print_usage'
)) {
    if ($moduleBase -notmatch [regex]::Escape($requiredPattern)) {
        throw "ModuleBase is missing CRTP call: $requiredPattern"
    }
}

foreach ($removedFile in @(
    'robot\common\Module.cpp',
    'robot\common\Result.hpp',
    'robot\common\Result.cpp'
)) {
    if (Test-Path -LiteralPath (Join-Path $projectRoot $removedFile)) {
        throw "Obsolete lifecycle file still exists: $removedFile"
    }
}

$rcS = Get-Content -LiteralPath (
    Join-Path $projectRoot 'startup\etc\init.d\rcS') -Raw
if ($rcS -notmatch '(?m)^robot start\s*$') {
    throw 'rcS does not start the robot runtime.'
}

$businessSources = Get-ChildItem -LiteralPath (
    Join-Path $projectRoot 'robot') -Recurse -File |
    Where-Object { $_.Extension -in @('.cpp', '.hpp') }
foreach ($source in $businessSources) {
    $text = Get-Content -LiteralPath $source.FullName -Raw
    if ($text -match '#include\s+[<"]stm32') {
        throw "Business code depends directly on STM32 headers: $($source.FullName)"
    }
    if ($text -match '\bWorkQueue\b') {
        throw "Step 3 must not depend on PX4 WorkQueue: $($source.FullName)"
    }
}

Write-Host '[PASS] Step 3 CRTP ModuleBase, OS wrappers, robot command, and rcS layout are complete.'
