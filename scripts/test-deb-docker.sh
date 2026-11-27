#!/bin/bash
#
# This file is part of the DSView project.
# DSView is based on PulseView.
#
# Copyright (C) 2026 Schildkroet
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301 USA
#
# Tests that a DSView .deb installs, starts and removes cleanly on the latest
# Ubuntu release and the two latest LTS releases, each in a throwaway Docker
# container. Nothing is installed on this machine.
#
# Usage:
#   scripts/test-deb-docker.sh [-i IMAGE]... [-v] [PACKAGE.deb]
#
#   PACKAGE.deb   the package to test; default: the newest build/*.deb
#   -i IMAGE      a Docker image to test on (repeatable); replaces the default
#                 list. Also: DSVIEW_TEST_IMAGES="ubuntu:24.04 debian:13"
#   -v            show the output of the passing tests as well
#   -h            this help
#
# Per image the script
#   1. installs the package with apt, which resolves its dependencies,
#   2. checks that every library of the program is found,
#   3. runs `DSView --version`, then starts DSView for a few seconds on the
#      offscreen Qt platform (this loads the Qt plugins and Python decoders),
#   4. removes and purges the package and checks that no files are left in the
#      folders it owns.
#
# The default images are "ubuntu:rolling" (the latest release, an LTS or not)
# and the two newest LTS releases. Update the LTS tags below when a new LTS
# comes out (every two years, in April).
#
# Environment:
#   DOCKER   the docker command; default "docker". Use DOCKER="sudo docker" if
#            your user may not talk to the Docker daemon.
#
# With Docker installed as a snap, the package must be under your home folder.
# Exit status: the number of images that failed.

set -u

DOCKER=${DOCKER:-docker}

# Latest release, then the two latest LTS releases.
DEFAULT_IMAGES="ubuntu:rolling ubuntu:26.04 ubuntu:24.04"

usage() {
    sed -n '/^# Tests that/,/^$/p' "$0" | sed 's/^# \{0,1\}//'
}

images=()
verbose=0
deb=""

while [ $# -gt 0 ]; do
    case "$1" in
        -i) [ $# -ge 2 ] || { echo "-i needs an image name" >&2; exit 2; }
            images+=("$2"); shift 2 ;;
        -v) verbose=1; shift ;;
        -h|--help) usage; exit 0 ;;
        -*) echo "unknown option: $1" >&2; usage >&2; exit 2 ;;
        *)  [ -z "$deb" ] || { echo "only one package can be tested" >&2; exit 2; }
            deb="$1"; shift ;;
    esac
done

if [ ${#images[@]} -eq 0 ]; then
    # shellcheck disable=SC2206  # word splitting of the list is intended
    images=(${DSVIEW_TEST_IMAGES:-$DEFAULT_IMAGES})
fi

if [ -z "$deb" ]; then
    root=$(cd "$(dirname "$0")/.." && pwd)
    deb=$(ls -t "$root"/build/*.deb 2>/dev/null | head -1)
    if [ -z "$deb" ]; then
        echo "No package given and no build/*.deb found. Run \`make deb\` first." >&2
        exit 2
    fi
fi

if [ ! -f "$deb" ]; then
    echo "Package not found: $deb" >&2
    exit 2
fi
deb=$(cd "$(dirname "$deb")" && pwd)/$(basename "$deb")

# shellcheck disable=SC2086  # $DOCKER may be "sudo docker"
if ! $DOCKER version >/dev/null 2>&1; then
    echo "Cannot talk to Docker (\`$DOCKER version\` failed)." >&2
    echo "Start the daemon, or set DOCKER=\"sudo docker\"." >&2
    exit 2
fi

# What runs inside the container. The package is mounted read-only.
read -r -d '' TEST_SCRIPT <<'EOF'
export DEBIAN_FRONTEND=noninteractive
fail() { echo "FAIL: $*"; exit 1; }

. /etc/os-release
echo "System: $PRETTY_NAME"

pkg=$(dpkg-deb -f /pkg/dsview-test.deb Package 2>/dev/null)
[ -n "$pkg" ] || pkg=dsview
echo "Package: $pkg $(dpkg-deb -f /pkg/dsview-test.deb Version 2>/dev/null)"

echo "--- 1. install"
apt-get update -qq || fail "apt-get update"
apt-get install -y -qq /pkg/dsview-test.deb || fail "the package does not install"
command -v DSView >/dev/null || fail "no DSView in the PATH after the install"

echo "--- 2. libraries"
# The bundled-Python package has a launcher script and the program as DSView.bin.
bin=/usr/bin/DSView.bin
[ -e "$bin" ] || bin=$(command -v DSView)
missing=$(ldd "$bin" 2>&1 | grep 'not found')
[ -z "$missing" ] || fail "libraries not found:
$missing"
echo "all libraries found"

echo "--- 3. start"
export QT_QPA_PLATFORM=offscreen
DSView --version || fail "DSView --version"
timeout 8 DSView >/tmp/dsview-start.log 2>&1
rc=$?
# 124: still running when the timeout stopped it, which is what we want.
if [ $rc -ne 124 ]; then
    tail -n 20 /tmp/dsview-start.log
    fail "DSView did not keep running (exit status $rc)"
fi
echo "DSView ran for 8 seconds"

echo "--- 4. remove and purge"
apt-get remove -y -qq "$pkg" || fail "apt-get remove"
apt-get purge -y -qq "$pkg" || fail "apt-get purge"
command -v DSView >/dev/null && fail "DSView is still in the PATH after the removal"
left=""
for d in /usr/share/DSView /usr/share/libsigrokdecode4DSL /usr/lib/dsview; do
    [ -e "$d" ] && left="$left $d"
done
[ -z "$left" ] || fail "left behind after the purge:$left"
echo "removed cleanly"
echo "PASS"
EOF

passed=()
failed=()

for image in "${images[@]}"; do
    echo
    echo "=== $image"
    # shellcheck disable=SC2086
    if ! $DOCKER pull -q "$image" >/dev/null; then
        echo "    could not pull the image"
        failed+=("$image (image not found)")
        continue
    fi

    # shellcheck disable=SC2086
    out=$($DOCKER run --rm -v "$deb":/pkg/dsview-test.deb:ro "$image" \
          bash -c "$TEST_SCRIPT" 2>&1)
    rc=$?

    if [ $rc -eq 0 ]; then
        passed+=("$image")
        [ $verbose -eq 1 ] && echo "$out" | sed 's/^/    /'
        echo "    PASS"
    else
        failed+=("$image")
        echo "$out" | sed 's/^/    /'
    fi
done

echo
echo "=== Summary for $(basename "$deb")"
for i in "${passed[@]}"; do echo "  PASS  $i"; done
for i in "${failed[@]}"; do echo "  FAIL  $i"; done

exit ${#failed[@]}
