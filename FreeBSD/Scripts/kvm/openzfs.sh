#!/bin/sh

# Stop on Error
set -e

# Setup Target Disk
# KVM virtio disks typically map to vtbd0. Change to nda0 if using NVMe.
DISK="vtbd0"

# Initialize GPT
gpart create -s gpt "${DISK}"

# Delete Partitions
# gpart delete -i 4 "${DISK}" || true
# gpart delete -i 3 "${DISK}" || true

# Create EFI Partition
gpart add -t efi -a 4k -s 200M "${DISK}"
newfs_msdos "/dev/${DISK}p1"

# Create ZFS Partition
gpart add -t freebsd-zfs -a 4k "${DISK}"

# Create ZFS Pool
zpool create -o altroot=/mnt -O compress=lz4 -O atime=off -m none zroot "/dev/${DISK}p2"

# Create Datasets
zfs create -o mountpoint=none zroot/ROOT
zfs create -o mountpoint=/ zroot/ROOT/default
zpool set bootfs=zroot/ROOT/default zroot
zfs create -o mountpoint=/home zroot/home

# Setup EFI Bootloader
mkdir -p /tmp/efi
mount -t msdosfs "/dev/${DISK}p1" /tmp/efi
mkdir -p /tmp/efi/efi/freebsd
mkdir -p /tmp/efi/efi/boot
cp /boot/loader.efi /tmp/efi/efi/freebsd/loader.efi
cp /boot/loader.efi /tmp/efi/efi/boot/bootx64.efi
umount /tmp/efi

# Setup fstab
mkdir -p /mnt/etc
touch /mnt/etc/fstab
