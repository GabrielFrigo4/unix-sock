#!/bin/sh

# Stop on Error
set -e

# Start FreeBSD (Daemon)
virsh --connect "qemu:///system" start FreeBSD

# Open VM Window
virt-viewer --connect "qemu:///system" FreeBSD

# List VMs IPs
virsh --connect "qemu:///system" net-dhcp-leases default

# Open SSH
ssh "gabriel@$(virsh --connect "qemu:///system" net-dhcp-leases default | grep "vmbsd" | awk '{print $(5)}' | cut -d'/' -f1)"
