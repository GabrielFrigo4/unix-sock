#!/usr/bin/env sh
# ----------------------------------------------------------------
# Utility: Partition Disk and Bootstrap OpenZFS on FreeBSD
# ----------------------------------------------------------------
set -eu

DISK="${1:-vtbd0}"

gpart create -s gpt "${DISK}"
gpart add -t efi -a 4k -s 200M "${DISK}"
newfs_msdos "/dev/${DISK}p1"

gpart add -t freebsd-zfs -a 4k "${DISK}"
zpool create -o altroot=/mnt -O compress=lz4 -O atime=off -m none zroot "/dev/${DISK}p2"

zfs create -o mountpoint=none zroot/ROOT
zfs create -o mountpoint=/ zroot/ROOT/default
zpool set bootfs=zroot/ROOT/default zroot
zfs create -o mountpoint=/home zroot/home

mkdir -p /tmp/efi
mount -t msdosfs "/dev/${DISK}p1" /tmp/efi
mkdir -p /tmp/efi/efi/freebsd
mkdir -p /tmp/efi/efi/boot
cp /boot/loader.efi /tmp/efi/efi/freebsd/loader.efi
cp /boot/loader.efi /tmp/efi/efi/boot/bootx64.efi
umount /tmp/efi

mkdir -p /mnt/etc
touch /mnt/etc/fstab
