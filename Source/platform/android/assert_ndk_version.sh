#!/bin/bash

# Bash script to assert that the current version of the NDK is at least the
# specified version. Prints 'true' to standard out if it's the right version,
# 'false' if it's not.
#
# Typically used like this, in your jni/Android.mk:
#
#   ifneq ($(shell $(LOCAL_PATH)/assert_ndk_version.sh "r5c"),true)
#     $(error NDK version r5c or greater required)
#   endif
#
# See https://gist.github.com/2878774 for asserting SDK version.
#
# Copyright 2012, Lookout, Inc. <jtjerno@mylookout.com>
# Licensed under the BSD license. See the zLICENSE file for information.
#
# Includes contributions from alexs.mac@gmail.com (Alex Stewart)
# and Philip Lamb.

# Extracts 'r5c' into '5 c', also handles newer versions of the form
# 'r9d (64-bit)' and versions >= 10.
function get_major_minor() {
  # r9d (64-bit) -> '9d', also handle versions >= 10.
  local version=$(echo "$1" | sed 's/r\([0-9]\{1,2\}[a-z]\{0,1\}\).*/\1/')
  local major=$(echo "$version" | sed 's/\([0-9]\{1,2\}\).*/\1/')
  local minor=$(echo "$version" | sed 's/^[0-9]*//')
  echo "$major $minor"
}

if [[ -z "$1" ]]; then
  echo "Usage: $0 <required version>" >&2
  echo " For example: $0 r5c" >&2
  exit 1
fi

# Assert that the expected version is at least 4.
declare -a expected_version
expected_version=( $(get_major_minor "$1") )
if [[ ${expected_version[0]} -le 4 ]]; then
  echo "Cannot test for versions less than r5: r4 doesn't have a version file." >&2
  echo false
  exit 1
fi

if [[ ! -d "$ANDROID_NDK_ROOT" ]]; then
  if [[ -d "$ANDROID_NDK" ]]; then
     ANDROID_NDK_ROOT=$ANDROID_NDK
  else 
    # Attempt to find ndk-build on the path.
    ANDROID_NDK_ROOT=$(dirname $(which ndk-build) 2>/dev/null)
  fi
  if [ ! -s "$ANDROID_NDK_ROOT" ]; then
    echo "Failed to find either ANDROID_NDK_ROOT or ndk-build."
    echo false
    exit 1
  fi
fi

source_properties="$ANDROID_NDK_ROOT/source.properties"
release_file="$ANDROID_NDK_ROOT/RELEASE.TXT"

# NDK version r11 and later encode NDK version in source.properties.
if [ -s "$source_properties" ]; then
  version=$(sed -En -e 's/^Pkg.Revision[ \t]*=[ \t]*([0-9a-f]+)/r\1/p' $source_properties)
elif [ -s "$release_file" ]; then
  version=$(grep '^r' $release_file)
else
  # NDK version r4 or earlier doesn't have a RELEASE.txt, and we just asserted
  # that the person was looking for r5 or above, so that implies that this is an
  # invalid version.
  echo false
  exit 0
fi

# Make sure the data is at least kinda sane.
declare -a actual_version
actual_version=( $(get_major_minor "$version") )
if [ -z "$version" ] || [ -z "${actual_version[0]}" ]; then
  echo "Invalid RELEASE.txt: $(cat $release_file)" >&2
  echo false
  exit 1
fi

if [[ ${actual_version[0]} -lt ${expected_version[0]} ]]; then
  echo "false"
elif [[ ${actual_version[0]} -eq ${expected_version[0]} ]]; then
  # This uses < and not -lt because they're string identifiers (a, b, c, etc)
  if [[ "${actual_version[1]}" < "${expected_version[1]}" ]]; then
    echo "false"
  else
    echo "true"
  fi
else
  echo "true"
fi
