#!/bin/sh

# Stop on Error
set -e

# Setup ISO
sudo mv "FreeBSD.iso" "/var/lib/libvirt/images/"

# FreeBSD VER
FREEBSD_VER="$(osinfo-query os | awk '$(1) ~ /^freebsd/ {print $(1)}' | sort -V | tail -n 1)"

# Install VM
virt-install \
  --connect qemu:///system \
  --name="FreeBSD" \
  --os-variant="${FREEBSD_VER}" \
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
