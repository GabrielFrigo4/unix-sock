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
  --memory=4096 \
  --disk size=32,format=qcow2,bus=virtio \
  --network network=default,model=virtio \
  --video qxl \
  --channel spicevmc \
  --channel unix,target_type=virtio,name=org.qemu.guest_agent.0 \
  --boot uefi \
  --cdrom="/var/lib/libvirt/images/FreeBSD.iso"

# Remove ISO
sudo rm "/var/lib/libvirt/images/FreeBSD.iso"
