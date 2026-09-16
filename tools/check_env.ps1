[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$failed = $false

function Find-ArmTool {
    param([Parameter(Mandatory)][string]$Pattern)

    $command = Get-Command $Pattern -ErrorAction SilentlyContinue
    if ($null -ne $command) {
        return $command.Source
    }

    $roots = @('C:\ST', 'D:\ST', 'C:\Program Files\STMicroelectronics', 'D:\Program Files\STMicroelectronics')
    $matches = foreach ($root in $roots) {
        if (Test-Path -LiteralPath $root) {
            Get-ChildItem -LiteralPath $root -Recurse -Filter $Pattern -File -ErrorAction SilentlyContinue
        }
    }

    return $matches | Sort-Object FullName -Descending | Select-Object -First 1 -ExpandProperty FullName
}

function Report-Tool {
    param([string]$Name, [string]$Path)

    if ($Path) {
        Write-Host "  [OK]      $Name -> $Path"
    } else {
        Write-Host "  [MISSING] $Name"
        $script:failed = $true
    }
}

Write-Host "Project: $projectRoot"
Write-Host 'Windows host tools:'
foreach ($name in @('curl.exe', 'tar.exe')) {
    $command = Get-Command $name -ErrorAction SilentlyContinue
    Report-Tool -Name $name -Path $(if ($command) { $command.Source } else { $null })
}

$msysBash = 'C:\msys64\usr\bin\bash.exe'
$kconfig = Join-Path $projectRoot 'tools\host\kconfig-frontends\bin\kconfig-conf.exe'
Write-Host 'NuttX MSYS2 host environment:'
Report-Tool -Name 'MSYS2 bash' -Path $(if (Test-Path -LiteralPath $msysBash) { $msysBash } else { $null })
Report-Tool -Name 'kconfig-conf' -Path $(if (Test-Path -LiteralPath $kconfig) { $kconfig } else { $null })

if (Test-Path -LiteralPath $msysBash) {
    & $msysBash -lc 'command -v python3 >/dev/null && command -v make >/dev/null && command -v flex >/dev/null && command -v bison >/dev/null'
    if ($LASTEXITCODE -eq 0) {
        Write-Host '  [OK]      make, Python, Flex and Bison'
    } else {
        Write-Host '  [MISSING] one or more MSYS2 build packages'
        $failed = $true
    }
}

Write-Host 'Arm cross compiler:'
Report-Tool -Name 'arm-none-eabi-gcc' -Path (Find-ArmTool -Pattern 'arm-none-eabi-gcc.exe')
Report-Tool -Name 'arm-none-eabi-g++' -Path (Find-ArmTool -Pattern 'arm-none-eabi-g++.exe')

if ($failed) {
    Write-Host 'Run .\tools\setup_build_env.ps1, and install STM32CubeIDE if Arm GCC is missing.'
    exit 1
}

Write-Host '[PASS] The Windows NuttX build environment is ready.'
