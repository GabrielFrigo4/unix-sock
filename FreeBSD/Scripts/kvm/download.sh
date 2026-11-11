#!/usr/bin/env sh
# ----------------------------------------------------------------
# Utility: Download and Verify FreeBSD Release ISO Image
# ----------------------------------------------------------------
set -eu

BASE_URL="https://download.freebsd.org/releases/amd64/amd64"
FREEBSD_VER="$(curl -sL "${BASE_URL}/" | sed -n 's/.*href="\([0-9]\+\.[0-9]\+\)-RELEASE\/".*/\1/p' | sort -V | tail -n 1)"
FREEBSD_URL="${BASE_URL}/ISO-IMAGES"
ISO_NAME="FreeBSD-${FREEBSD_VER}-RELEASE-amd64-disc1.iso.xz"

curl -o "FreeBSD.iso.xz" "${FREEBSD_URL}/${FREEBSD_VER}/${ISO_NAME}"
curl -sL "${FREEBSD_URL}/${FREEBSD_VER}/CHECKSUM.SHA256-FreeBSD-${FREEBSD_VER}-RELEASE-amd64" | \
	grep "${ISO_NAME}" | awk '{print $(4) "  FreeBSD.iso.xz"}' | sha256sum -c -
unxz -f "FreeBSD.iso.xz"
