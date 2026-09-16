#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
export PATH="$project_root/tools/host/bin:$project_root/tools/host/kconfig-frontends/bin:$PATH"
source "$project_root/config/versions.env"
clean=0
host_msys=0
jobs="$(getconf _NPROCESSORS_ONLN 2>/dev/null || printf '1')"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --clean) clean=1; shift ;;
        --host-msys) host_msys=1; shift ;;
        --jobs)
            [[ $# -ge 2 ]] || { printf 'ERROR: --jobs requires a value.\n' >&2; exit 2; }
            jobs="$2"
            shift 2
            ;;
        *) printf 'ERROR: unknown argument: %s\n' "$1" >&2; exit 2 ;;
    esac
done

case "$jobs" in
    ''|*[!0-9]*|0) printf 'ERROR: jobs must be a positive integer.\n' >&2; exit 2 ;;
esac

"$project_root/tools/check_env.sh"
"$project_root/tools/bootstrap.sh"

nuttx="$project_root/upstream/nuttx"
board_config="$project_root/platform/$NUTTX_BOARD/configs/$NUTTX_CONFIG/defconfig"
artifacts="$project_root/artifacts"

# Keep project-owned applications outside the downloaded Apache apps tree.
# A clean staging copy is created for every build so upstream remains disposable.

robot_app="$project_root/upstream/apps/cboard"
generated_msg="$project_root/build/generated/msg"
python3 "$project_root/tools/generate_messages.py" \
    --input "$project_root/msg" --output "$generated_msg"
rm -rf "$robot_app"
mkdir -p "$robot_app"
cp -R "$project_root/apps/cboard/." "$robot_app/"
cp -R "$generated_msg" "$robot_app/msg"
cp -R "$project_root/robot" "$robot_app/robot"
cp -R "$project_root/protocol" "$robot_app/protocol"

# Generate the read-only /etc image containing init.d/rcS.

romfs_image="$project_root/build/cboard-etc.img"
romfs_source="$project_root/startup/etc"
romfs_c="$project_root/platform/$NUTTX_BOARD/src/etc_romfs.c"
mkdir -p "$project_root/build"
genromfs -f "$romfs_image" -d "$romfs_source" -V cboard_etc
python3 "$project_root/tools/generate_romfs.py" "$romfs_image" "$romfs_c"

if [[ ! -f "$board_config" ]]; then
    printf 'ERROR: missing board configuration: %s\n' "$board_config" >&2
    exit 3
fi

if [[ "$clean" -eq 1 && -f "$nuttx/Makefile" ]]; then
    make -C "$nuttx" distclean
fi

configure_config="$board_config"
if [[ "$host_msys" -eq 1 ]]; then
    configure_config="$project_root/build/msys-config"
    mkdir -p "$configure_config"
    cp "$board_config" "$configure_config/defconfig"
    cp "$project_root/platform/$NUTTX_BOARD/scripts/Make.defs" \
        "$configure_config/Make.defs"
    {
        printf '\nCONFIG_HOST_WINDOWS=y\n'
        printf 'CONFIG_WINDOWS_MSYS=y\n'
    } >> "$configure_config/defconfig"
fi

configure_args=(-a ../apps "$configure_config")
if [[ "$clean" -eq 1 ]]; then
    configure_args=(-E "${configure_args[@]}")
fi

bash "$nuttx/tools/configure.sh" "${configure_args[@]}"

# NuttX copies the selected board sources into arch/<arch>/src/board only when
# the build context is created.  Refresh the generated ROMFS source explicitly
# so an incremental build cannot retain an older /etc image.
board_romfs_c="$nuttx/arch/arm/src/board/etc_romfs.c"
board_romfs_o="$nuttx/arch/arm/src/board/etc_romfs.o"
if [[ -d "$(dirname "$board_romfs_c")" ]]; then
    cp "$romfs_c" "$board_romfs_c"
    rm -f "$board_romfs_o"
fi

make -C "$nuttx" -j"$jobs"

mkdir -p "$artifacts"
for output in nuttx nuttx.bin nuttx.hex nuttx.map; do
    if [[ -f "$nuttx/$output" ]]; then
        cp "$nuttx/$output" "$artifacts/$output"
    fi
done

arm-none-eabi-size "$nuttx/nuttx" > "$artifacts/size.txt"
{
    printf 'board=%s\n' "$NUTTX_BOARD"
    printf 'config=%s\n' "$NUTTX_CONFIG"
    printf 'nuttx_version=%s\n' "$NUTTX_VERSION"
    printf 'nuttx_commit=%s\n' "$NUTTX_COMMIT"
    printf 'nuttx_archive_sha512=%s\n' "$NUTTX_ARCHIVE_SHA512"
    printf 'nuttx_apps_version=%s\n' "$NUTTX_APPS_VERSION"
    printf 'nuttx_apps_commit=%s\n' "$NUTTX_APPS_COMMIT"
    printf 'nuttx_apps_archive_sha512=%s\n' "$NUTTX_APPS_ARCHIVE_SHA512"
    printf 'compiler=%s\n' "$(arm-none-eabi-gcc --version | head -n 1)"
    printf 'build_utc=%s\n' "$(date -u +%Y-%m-%dT%H:%M:%SZ)"
} > "$artifacts/build-info.txt"

printf '[PASS] Firmware artifacts written to %s\n' "$artifacts"
