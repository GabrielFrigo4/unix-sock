#!/bin/sh

# Stop on Error
set -e

# Reboot VM
virsh --connect "qemu:///system" reboot FreeBSD
