#!/bin/sh

# Stop on Error
set -e

# Start FreeBSD (Daemon)
virsh --connect "qemu:///system" start FreeBSD-15.0

# Open VM Window
virt-viewer --connect "qemu:///system" FreeBSD-15.0

# List VMs IPs
virsh --connect "qemu:///system" net-dhcp-leases default

# Open SSH
ssh "gabriel@$(virsh --connect "qemu:///system" net-dhcp-leases default | grep "vmbsd" | awk '{print ${5}}' | cut -d'/' -f1)"
