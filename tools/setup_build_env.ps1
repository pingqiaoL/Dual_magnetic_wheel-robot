[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$msysRoot = 'C:\msys64'
$bash = Join-Path $msysRoot 'usr\bin\bash.exe'

if (-not (Test-Path -LiteralPath $bash -PathType Leaf)) {
    throw 'MSYS2 is not installed at C:\msys64. Install it before running this package setup.'
}

Write-Host 'Updating MSYS2 package databases and core packages...'
& $bash -lc 'pacman --noconfirm -Syuu'
if ($LASTEXITCODE -ne 0) {
    throw 'The first MSYS2 update pass failed.'
}

& $bash -lc 'pacman --noconfirm -Syuu'
if ($LASTEXITCODE -ne 0) {
    throw 'The second MSYS2 update pass failed.'
}

Write-Host 'Installing NuttX host build dependencies...'
& $bash -lc 'pacman --noconfirm --needed -S base-devel gcc git python python-pip ncurses-devel unzip zip gperf autoconf automake libtool genromfs'
if ($LASTEXITCODE -ne 0) {
    throw 'Installing the NuttX host dependencies failed.'
}

$env:CBOARD_PROJECT_ROOT = $projectRoot
$installKconfig = @'
set -euo pipefail
project_root="$(cygpath -u "$CBOARD_PROJECT_ROOT")"
prefix="$project_root/tools/host/kconfig-frontends"

if [[ ! -x "$prefix/bin/kconfig-conf.exe" ]]; then
  source_dir="$project_root/build/kconfig-tools-src"
  rm -rf "$source_dir"
  git clone --depth 1 https://github.com/patacongo/tools.git "$source_dir"
  cd "$source_dir/kconfig-frontends"
  ./configure --prefix="$prefix" \
    --disable-kconfig --disable-nconf --disable-qconf \
    --disable-gconf --disable-mconf --disable-static \
    --disable-shared --disable-L10n
  touch aclocal.m4 Makefile.in configure
  find . -name Makefile.in -exec touch {} +
  make -j"$(nproc)"
  make install
fi
'@

Write-Host 'Installing the NuttX Kconfig command-line frontend...'
& $bash -lc $installKconfig
if ($LASTEXITCODE -ne 0) {
    throw 'Building the NuttX Kconfig frontend failed.'
}

Write-Host '[PASS] MSYS2 NuttX host environment is ready.'
