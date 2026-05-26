#! /bin/bash

#
# Sign one or more APKs using a keystore configured via environment variables.
#
# Copyright 2026, artoolkitX Contributors.
#
# Environment variables (each defaults to the Android-standard debug.keystore
# values when unset, so an unconfigured invocation signs with the local debug
# keystore):
#   ANDROID_KEYSTORE_PATH            Path to the keystore file (JKS or PKCS12).
#                                    Default: ~/.android/debug.keystore
#   ANDROID_KEYSTORE_PASSWORD        Password protecting the keystore.
#                                    Default: android
#   ANDROID_KEYSTORE_ALIAS           Alias under which the signing key is stored.
#                                    Default: androiddebugkey
#   ANDROID_KEYSTORE_ALIAS_PASSWORD  Password protecting the signing key itself.
#                                    Default: android
#
# Usage:
#   sign-apks.sh <apk> [<apk> ...]
#
# For each input "Foo-release-unsigned.apk" a signed file
# "Foo-release-signed.apk" is written next to it (or "<base>-signed.apk"
# if the input name does not end in "-unsigned").
#

set -e

function usage {
    cat >&2 <<EOF
Usage: $(basename "$0") <apk> [<apk> ...]

If unset, the following environment variables default to the Android-standard
debug.keystore values:
  ANDROID_KEYSTORE_PATH             (default: ~/.android/debug.keystore)
  ANDROID_KEYSTORE_PASSWORD         (default: android)
  ANDROID_KEYSTORE_ALIAS            (default: androiddebugkey)
  ANDROID_KEYSTORE_ALIAS_PASSWORD   (default: android)
EOF
    exit 1
}

if [ $# -lt 1 ]; then
    usage
fi

# Default any unset env var to the Android-standard debug.keystore value, so an
# unconfigured invocation signs with the local debug keystore.
: "${ANDROID_KEYSTORE_PATH:=$HOME/.android/debug.keystore}"
: "${ANDROID_KEYSTORE_PASSWORD:=android}"
: "${ANDROID_KEYSTORE_ALIAS:=androiddebugkey}"
: "${ANDROID_KEYSTORE_ALIAS_PASSWORD:=android}"
export ANDROID_KEYSTORE_PASSWORD ANDROID_KEYSTORE_ALIAS_PASSWORD

if [ ! -f "$ANDROID_KEYSTORE_PATH" ]; then
    echo "Keystore not found: $ANDROID_KEYSTORE_PATH" >&2
    exit 1
fi

# Locate apksigner: prefer one on PATH, otherwise use the highest-versioned
# build-tools install under ANDROID_HOME / ANDROID_SDK_ROOT.
APKSIGNER="$(command -v apksigner || true)"
if [ -z "$APKSIGNER" ]; then
    SDK_ROOT="${ANDROID_HOME:-${ANDROID_SDK_ROOT:-}}"
    if [ -z "$SDK_ROOT" ]; then
        echo "apksigner not on PATH and neither ANDROID_HOME nor ANDROID_SDK_ROOT is set." >&2
        exit 1
    fi
    APKSIGNER="$(find "$SDK_ROOT/build-tools" -mindepth 2 -maxdepth 2 -type f -name apksigner 2>/dev/null | sort -V | tail -n1)"
    if [ -z "$APKSIGNER" ] || [ ! -x "$APKSIGNER" ]; then
        echo "apksigner not found under $SDK_ROOT/build-tools/" >&2
        exit 1
    fi
fi
echo "Using apksigner: $APKSIGNER"

# Sign each APK.
for apk in "$@"; do
    if [ ! -f "$apk" ]; then
        echo "APK not found: $apk" >&2
        exit 1
    fi

    dir="$(dirname "$apk")"
    name="$(basename "$apk")"
    # Convention: "<base>-unsigned.apk" -> "<base>-signed.apk"; otherwise append "-signed".
    case "$name" in
        *-unsigned.apk) out_name="${name%-unsigned.apk}-signed.apk" ;;
        *.apk)          out_name="${name%.apk}-signed.apk" ;;
        *)              out_name="${name}-signed.apk" ;;
    esac
    out="$dir/$out_name"

    echo "Signing: $apk"
    echo "  ->     $out"
    "$APKSIGNER" sign \
        --ks "$ANDROID_KEYSTORE_PATH" \
        --ks-pass "env:ANDROID_KEYSTORE_PASSWORD" \
        --ks-key-alias "$ANDROID_KEYSTORE_ALIAS" \
        --key-pass "env:ANDROID_KEYSTORE_ALIAS_PASSWORD" \
        --out "$out" \
        "$apk"

    "$APKSIGNER" verify --print-certs "$out" | head -n 4
    echo
done
