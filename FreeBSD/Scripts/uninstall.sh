#!/bin/sh

# Stop on Error
set -e

# Uninstall VM
virsh --connect "qemu:///system" destroy FreeBSD
virsh --connect "qemu:///system" undefine FreeBSD --remove-all-storage --nvram
