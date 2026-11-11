#!/usr/bin/env sh
# ----------------------------------------------------------------
# Utility: Connect and Manage FreeBSD Virtual Machine via Libvirt
# ----------------------------------------------------------------
set -eu

VM_NAME="${1:-FreeBSD}"

virsh --connect "qemu:///system" start "${VM_NAME}" 2> "/dev/null" || true
virt-viewer --connect "qemu:///system" "${VM_NAME}" 2> "/dev/null" || true
virsh --connect "qemu:///system" console "${VM_NAME}" 2> "/dev/null" || true
virsh --connect "qemu:///system" net-dhcp-leases default
virsh --connect "qemu:///system" domifaddr "${VM_NAME}"

VM_IP="$(virsh --connect "qemu:///system" domifaddr "${VM_NAME}" | awk '$(3) == "ipv4" {print $(4)}' | cut -d'/' -f1)"
if [ -n "${VM_IP}" ]; then
	ssh "gabriel@${VM_IP}"
fi
