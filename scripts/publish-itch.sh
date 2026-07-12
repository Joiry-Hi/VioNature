#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "$0")/.." && pwd)"
cd "$ROOT"

if [ -z "${BUTLER:-}" ] && [ -x "$ROOT/tools/butler/butler" ]; then
    BUTLER="$ROOT/tools/butler/butler"
else
    BUTLER="${BUTLER:-butler}"
fi
ITCH_TARGET="${ITCH_TARGET:-}"
VERSION="${VERSION:-}"
BUTLER_IDENTITY="${BUTLER_IDENTITY:-${HOME:-}/.config/itch/butler_creds}"

WINDOWS_CHANNEL="${WINDOWS_CHANNEL:-windows}"
LINUX_CHANNEL="${LINUX_CHANNEL:-linux}"
WEB_CHANNEL="${WEB_CHANNEL:-html5}"

WINDOWS_RELEASE_DIR="${WINDOWS_RELEASE_DIR:-build-windows/release}"
LINUX_RELEASE_DIR="${LINUX_RELEASE_DIR:-build-sandbox/release-linux-en}"
WEB_RELEASE_DIR="${WEB_RELEASE_DIR:-web-release/dist}"

SKIP_BUILD=0
CHECK_ONLY=0
REQUESTED=()

usage() {
    cat <<'EOF'
Usage:
  ITCH_TARGET=user/game bash scripts/publish-itch.sh [all|windows|linux|web] [--skip-build] [--check]

Examples:
  ITCH_TARGET=joiry-hi/vionature bash scripts/publish-itch.sh all
  ITCH_TARGET=joiry-hi/vionature VERSION=0.3.1 bash scripts/publish-itch.sh web
  ITCH_TARGET=joiry-hi/vionature bash scripts/publish-itch.sh windows linux --skip-build
  ITCH_TARGET=joiry-hi/vionature bash scripts/publish-itch.sh all --check

Environment:
  ITCH_TARGET          Required itch.io target in user/game form, from your
                       project URL: https://user.itch.io/game
  VERSION              Optional user-facing version passed to butler.
  BUTLER               Optional butler executable path. Default: butler
  BUTLER_API_KEY       Optional itch.io API key for CI or if browser login fails.
  BUTLER_IDENTITY      Optional butler credential file path.
  WINDOWS_CHANNEL      Default: windows
  LINUX_CHANNEL        Default: linux
  WEB_CHANNEL          Default: html5
  WINDOWS_RELEASE_DIR  Default: build-windows/release
  LINUX_RELEASE_DIR    Default: build-sandbox/release-linux-en
  WEB_RELEASE_DIR      Default: web-release/dist

Notes:
  - Desktop channels upload release folders, not zip archives, so butler can
    generate smaller patches for repeat uploads.
  - Web uploads web-release/dist/ directly, with index.html at the channel root.
  - Run `butler login` once beforehand, or set BUTLER_API_KEY in CI.
  - Use --check to validate credentials/paths before spending time building.
EOF
}

while (($#)); do
    case "$1" in
        -h|--help)
            usage
            exit 0
            ;;
        --skip-build)
            SKIP_BUILD=1
            ;;
        --check)
            CHECK_ONLY=1
            ;;
        all|windows|linux|web)
            REQUESTED+=("$1")
            ;;
        *)
            echo "Unknown argument: $1" >&2
            usage >&2
            exit 2
            ;;
    esac
    shift
done

if [ ${#REQUESTED[@]} -eq 0 ]; then
    REQUESTED=(all)
fi

EXPANDED=()
for item in "${REQUESTED[@]}"; do
    if [ "$item" = "all" ]; then
        EXPANDED+=(windows linux web)
    else
        EXPANDED+=("$item")
    fi
done

if [ -z "$ITCH_TARGET" ]; then
    echo "ITCH_TARGET is required, e.g. ITCH_TARGET=joiry/vionature" >&2
    exit 2
fi

if [[ "$ITCH_TARGET" != */* || "$ITCH_TARGET" == *:* || "$ITCH_TARGET" == *" "* ]]; then
    echo "ITCH_TARGET must be in itch.io user/game form, e.g. joiry-hi/vionature" >&2
    echo "Current value: $ITCH_TARGET" >&2
    exit 2
fi

if [[ "${ITCH_TARGET,,}" == *slug* ]]; then
    echo "ITCH_TARGET contains the word 'slug'; did you accidentally paste the placeholder text?" >&2
    echo "Use the actual itch.io project URL slug, e.g. joiry-hi/vionature." >&2
    echo "Current value: $ITCH_TARGET" >&2
    exit 2
fi

if ! command -v "$BUTLER" >/dev/null 2>&1; then
    echo "butler was not found. Install it from itch.io, then run \`butler login\`." >&2
    echo "You can also set BUTLER=/path/to/butler." >&2
    exit 127
fi

check_butler_auth() {
    if [ -n "${BUTLER_API_KEY:-}" ]; then
        return
    fi
    if [ -f "$BUTLER_IDENTITY" ]; then
        return
    fi

    cat >&2 <<EOF
butler is not logged in yet.

Run one of:
  $BUTLER login

or, if OAuth/network login is unreliable:
  1. Create an API key in itch.io account settings.
  2. Publish with:
     BUTLER_API_KEY=your_key ITCH_TARGET=$ITCH_TARGET bash scripts/publish-itch.sh ${REQUESTED[*]}

Credential file expected at:
  $BUTLER_IDENTITY
EOF
    exit 2
}

check_release_paths() {
    local platform source_path
    for platform in "${EXPANDED[@]}"; do
        source_path="$(path_for "$platform")"
        if [ "$SKIP_BUILD" -eq 1 ] && [ ! -d "$source_path" ]; then
            echo "Missing release directory for $platform: $source_path" >&2
            exit 1
        fi
    done
}

run_build() {
    if [ "$SKIP_BUILD" -eq 1 ]; then
        return
    fi

    case "$1" in
        windows)
            bash scripts/package-release.sh
            ;;
        linux)
            bash scripts/package-release-linux-en.sh
            ;;
        web)
            bash scripts/build-web.sh
            ;;
    esac
}

channel_for() {
    case "$1" in
        windows) printf '%s' "$WINDOWS_CHANNEL" ;;
        linux) printf '%s' "$LINUX_CHANNEL" ;;
        web) printf '%s' "$WEB_CHANNEL" ;;
    esac
}

path_for() {
    case "$1" in
        windows) printf '%s' "$WINDOWS_RELEASE_DIR" ;;
        linux) printf '%s' "$LINUX_RELEASE_DIR" ;;
        web) printf '%s' "$WEB_RELEASE_DIR" ;;
    esac
}

push_channel() {
    local platform="$1"
    local channel
    local source_path
    channel="$(channel_for "$platform")"
    source_path="$(path_for "$platform")"

    if [ ! -d "$source_path" ]; then
        echo "Missing release directory: $source_path" >&2
        echo "Run without --skip-build, or build/package $platform first." >&2
        exit 1
    fi

    local target="$ITCH_TARGET:$channel"
    echo ""
    echo "==> Uploading $platform"
    echo "    source: $source_path"
    echo "    target: $target"

    if [ -n "$VERSION" ]; then
        "$BUTLER" push "$source_path" "$target" --userversion "$VERSION"
    else
        "$BUTLER" push "$source_path" "$target"
    fi
}

echo "========================================"
echo "  VioNature itch.io Publisher"
echo "========================================"
echo "Target: $ITCH_TARGET"
if [ -n "$VERSION" ]; then
    echo "Version: $VERSION"
fi
echo "Channels: ${EXPANDED[*]}"
echo "Butler: $BUTLER"

check_butler_auth
check_release_paths

if [ "$CHECK_ONLY" -eq 1 ]; then
    echo ""
    echo "Check OK. Planned uploads:"
    for platform in "${EXPANDED[@]}"; do
        echo "  $(path_for "$platform") -> $ITCH_TARGET:$(channel_for "$platform")"
    done
    echo ""
    echo "Re-run without --check to build/upload."
    exit 0
fi

for platform in "${EXPANDED[@]}"; do
    run_build "$platform"
    push_channel "$platform"
done

echo ""
echo "========================================"
echo "  itch.io upload complete"
echo "========================================"
