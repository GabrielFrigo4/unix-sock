#!/bin/sh

# Stop on Error
set -e

# Setup ISO
sudo mv "FreeBSD.iso" "/var/lib/libvirt/images/"

# Install VM
virt-install \
  --connect qemu:///system \
  --name="FreeBSD-15.0" \
  --os-variant="freebsd15.0" \
  --vcpus=4 \
  --memory=8192 \
  --disk size=32,format=qcow2 \
  --network network=default \
  --video qxl \
  --channel spicevmc \
  --boot uefi \
  --cdrom="/var/lib/libvirt/images/FreeBSD.iso"

# Remove ISO
sudo rm "/var/lib/libvirt/images/FreeBSD.iso"
