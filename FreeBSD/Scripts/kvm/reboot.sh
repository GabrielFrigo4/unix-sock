#!/usr/bin/env sh
# ----------------------------------------------------------------
# Utility: Reboot FreeBSD Virtual Machine via Libvirt
# ----------------------------------------------------------------
set -eu

VM_NAME="${1:-FreeBSD}"

virsh --connect "qemu:///system" reboot "${VM_NAME}"
