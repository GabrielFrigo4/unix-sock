#!/bin/sh

# Stop on Error
set -e

# Close FreeBSD (Daemon)
virsh --connect "qemu:///system" destroy FreeBSD

# Uninstall VM
virsh --connect "qemu:///system" undefine FreeBSD --remove-all-storage --nvram
