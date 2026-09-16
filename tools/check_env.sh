#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
failed=0

printf 'Project: %s\n' "$project_root"
for command_name in python3 make arm-none-eabi-gcc arm-none-eabi-g++ curl tar sha512sum genromfs; do
    if command -v "$command_name" >/dev/null 2>&1; then
        printf '  [OK]      %-22s %s\n' "$command_name" "$(command -v "$command_name")"
    else
        printf '  [MISSING] %s\n' "$command_name"
        failed=1
    fi
done

exit "$failed"
