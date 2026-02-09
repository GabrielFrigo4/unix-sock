#!/bin/sh

# Stop on Error
set -e

# Environment FreeBSD
FREEBSD_URL="https://download.freebsd.org/releases/amd64/amd64/ISO-IMAGES"
FREEBSD_VER="$(curl -s "$FREEBSD_URL/" | sed -n 's/.*href="\([0-9]\+\.[0-9]\+\)\/".*/\1/p' | sort -V | tail -n 1)"
ISO_NAME="FreeBSD-$FREEBSD_VER-RELEASE-amd64-disc1.iso.xz"

# Download FreeBSD
curl -o "FreeBSD.iso.xz" "$FREEBSD_URL/$FREEBSD_VER/$ISO_NAME"

# Verify Checksum
curl -s "$FREEBSD_URL/$FREEBSD_VER/CHECKSUM.SHA256-FreeBSD-$FREEBSD_VER-RELEASE-amd64" | \
    grep "$ISO_NAME" | awk '{print $4 "  FreeBSD.iso.xz"}' | sha256sum -c -

# Extract FreeBSD
unxz -f "FreeBSD.iso.xz"
