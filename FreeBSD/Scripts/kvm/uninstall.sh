#!/usr/bin/env sh
# ----------------------------------------------------------------
# Utility: Destroy and Undefine FreeBSD Virtual Machine
# ----------------------------------------------------------------
set -eu

VM_NAME="${1:-FreeBSD}"

virsh --connect "qemu:///system" destroy "${VM_NAME}"
virsh --connect "qemu:///system" undefine "${VM_NAME}" --remove-all-storage --nvram
