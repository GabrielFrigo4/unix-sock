#!/bin/sh

# Stop on Error
set -e

# Start FreeBSD (Daemon)
virsh --connect "qemu:///system" start FreeBSD

# Open VM Window
virt-viewer --connect "qemu:///system" FreeBSD

# Open VM Console
virsh --connect "qemu:///system" console FreeBSD

# List VMs IPs
virsh --connect "qemu:///system" net-dhcp-leases default

# List VM IPs
virsh --connect "qemu:///system" domifaddr FreeBSD

# Open VM SSH
ssh "gabriel@$(virsh --connect "qemu:///system" domifaddr FreeBSD | awk '$(3) == "ipv4" {print $(4)}' | cut -d'/' -f1)"
