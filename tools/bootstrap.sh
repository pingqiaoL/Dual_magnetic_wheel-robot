#!/usr/bin/env bash
set -euo pipefail

project_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
source "$project_root/config/versions.env"
mkdir -p "$project_root/upstream/.cache"

install_verified_archive() {
    local name="$1"
    local version="$2"
    local url="$3"
    local sha512="$4"
    local archive_name="$5"
    local extracted_name="$6"
    local destination="$7"
    local commit="$8"
    local marker_name='.cboard-source-version'
    local expected_marker="$name $version $sha512 $commit"

    if [[ -e "$destination" ]]; then
        if [[ ! -f "$destination/$marker_name" ]]; then
            printf 'ERROR: %s exists without %s; refusing to replace it.\n' "$destination" "$marker_name" >&2
            exit 1
        fi
        if [[ "$(cat "$destination/$marker_name")" != "$expected_marker" ]]; then
            printf 'ERROR: %s source marker mismatch; refusing to replace it.\n' "$name" >&2
            exit 1
        fi
        printf '[OK] %s %s (%s)\n' "$name" "$version" "$commit"
        return
    fi

    local archive="$project_root/upstream/.cache/$archive_name"
    if [[ ! -f "$archive" ]]; then
        printf 'Downloading %s %s...\n' "$name" "$version"
        if [[ -f "$archive.part" ]]; then
            curl -L --fail --retry 8 --retry-delay 3 -C - -o "$archive.part" "$url"
        else
            curl -L --fail --retry 8 --retry-delay 3 -o "$archive.part" "$url"
        fi
        mv "$archive.part" "$archive"
    fi

    printf '%s  %s\n' "$sha512" "$archive" | sha512sum --check --status
    local extract="${destination}.extract-$$"
    mkdir "$extract"
    tar -xzf "$archive" -C "$extract"
    if [[ ! -d "$extract/$extracted_name" ]]; then
        printf 'ERROR: expected archive root %s was not found.\n' "$extracted_name" >&2
        exit 1
    fi

    printf '%s' "$expected_marker" > "$extract/$extracted_name/$marker_name"
    mv "$extract/$extracted_name" "$destination"
    rmdir "$extract"
    printf '[OK] %s %s (%s)\n' "$name" "$version" "$commit"
}

install_verified_archive 'NuttX' "$NUTTX_VERSION" "$NUTTX_ARCHIVE_URL" \
    "$NUTTX_ARCHIVE_SHA512" "apache-nuttx-$NUTTX_VERSION.tar.gz" \
    "nuttx" "$project_root/upstream/nuttx" "$NUTTX_COMMIT"

install_verified_archive 'NuttX Apps' "$NUTTX_APPS_VERSION" "$NUTTX_APPS_ARCHIVE_URL" \
    "$NUTTX_APPS_ARCHIVE_SHA512" "apache-nuttx-apps-$NUTTX_APPS_VERSION.tar.gz" \
    "apps" "$project_root/upstream/apps" "$NUTTX_APPS_COMMIT"
