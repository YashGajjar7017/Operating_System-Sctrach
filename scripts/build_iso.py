#!/usr/bin/env python3
"""
@file build_iso.py
@brief Pure Python UEFI El Torito Bootable ISO 9660 Image Generator.
Produces a bootable .iso (AuraOS.iso) suitable for sharing, CD/DVD, USB, VirtualBox, VMware, and QEMU.
"""

import os
import sys
import struct
import shutil
from build_disk import FAT32Builder, build_uefi_disk

ISO_SECTOR_SIZE = 2048  # ISO 9660 sector size

def make_el_torito_catalog(efi_img_lba, efi_img_512_sectors):
    """
    Creates a 2048-byte El Torito Boot Catalog containing:
    1. Validation Entry
    2. Initial/Default Entry (Non-bootable placeholder)
    3. Section Header for EFI (Platform 0xEF)
    4. Section Entry for EFI System Partition (Bootable, No Emulation)
    """
    catalog = bytearray(ISO_SECTOR_SIZE)

    # 1. Validation Entry (32 bytes)
    val = bytearray(32)
    val[0] = 0x01                      # Header ID
    val[1] = 0x00                      # Platform ID (80x86)
    val[2:4] = b'\x00\x00'             # Reserved
    id_str = b'AuraOS ElTorito'
    val[4:4+len(id_str)] = id_str
    val[30] = 0x55                     # Key byte 1
    val[31] = 0xAA                     # Key byte 2

    # Calculate checksum: sum of 16-bit little endian words in 32-byte record must be 0
    words = struct.unpack('<16H', val)
    csum = (-sum(words)) & 0xFFFF
    struct.pack_into('<H', val, 28, csum)
    catalog[0:32] = val

    # 2. Initial / Default Entry (Legacy BIOS - Not Bootable)
    init_entry = bytearray(32)
    init_entry[0] = 0x00               # Boot indicator (0x00 = not bootable)
    init_entry[1] = 0x00               # No emulation
    catalog[32:64] = init_entry

    # 3. Section Header for UEFI (32 bytes)
    sec_hdr = bytearray(32)
    sec_hdr[0] = 0x91                  # Final section header
    sec_hdr[1] = 0xEF                  # Platform ID: 0xEF = EFI
    struct.pack_into('<H', sec_hdr, 2, 1) # 1 Section entry
    sec_id = b'EFI System Partition'
    sec_hdr[4:4+len(sec_id)] = sec_id
    catalog[64:96] = sec_hdr

    # 4. Section Entry for EFI Boot Image (32 bytes)
    sec_entry = bytearray(32)
    sec_entry[0] = 0x88                # Bootable (0x88)
    sec_entry[1] = 0x00                # Media type: 0 = No Emulation
    struct.pack_into('<H', sec_entry, 2, 0) # Load segment
    sec_entry[4] = 0x00                # System type
    sec_entry[5] = 0x00                # Unused
    struct.pack_into('<H', sec_entry, 6, min(efi_img_512_sectors, 0xFFFF)) # Sector count in 512-byte blocks
    struct.pack_into('<I', sec_entry, 8, efi_img_lba)                      # 2048-byte LBA of FAT ESP image
    catalog[96:128] = sec_entry

    return catalog

def create_iso_descriptor_set(total_iso_sectors, boot_catalog_lba):
    """Generates Primary Volume Descriptor, El Torito Boot Record, and Terminator."""
    pvd = bytearray(ISO_SECTOR_SIZE)
    # Primary Volume Descriptor (Type 1)
    pvd[0] = 0x01
    pvd[1:6] = b'CD001'
    pvd[6] = 0x01                      # Version
    pvd[7] = 0x00                      # Unused
    pvd[8:40] = b'AURAOS                          ' # System ID
    pvd[40:72] = b'AURAOS_INSTALL                  ' # Volume ID
    struct.pack_into('<I', pvd, 80, total_iso_sectors) # Volume Space Size (LSB)
    struct.pack_into('>I', pvd, 84, total_iso_sectors) # Volume Space Size (MSB)
    struct.pack_into('<H', pvd, 120, 1) # Volume Set Size
    struct.pack_into('>H', pvd, 122, 1)
    struct.pack_into('<H', pvd, 124, 1) # Volume Sequence Number
    struct.pack_into('>H', pvd, 126, 1)
    struct.pack_into('<H', pvd, 128, ISO_SECTOR_SIZE) # Logical Block Size
    struct.pack_into('>H', pvd, 130, ISO_SECTOR_SIZE)

    # El Torito Boot Record Volume Descriptor (Type 0)
    brvd = bytearray(ISO_SECTOR_SIZE)
    brvd[0] = 0x00
    brvd[1:6] = b'CD001'
    brvd[6] = 0x01
    brvd[7:39] = b'EL TORITO SPECIFICATION' + b'\x00' * 9
    struct.pack_into('<I', brvd, 71, boot_catalog_lba) # Boot Catalog LBA

    # Volume Descriptor Set Terminator (Type 255)
    vdt = bytearray(ISO_SECTOR_SIZE)
    vdt[0] = 0xFF
    vdt[1:6] = b'CD001'
    vdt[6] = 0x01

    return pvd, brvd, vdt

def build_uefi_iso(output_iso, efi_loader, kernel_elf):
    print(f"[*] Building bootable UEFI ISO: {output_iso}...")
    temp_fat_img = "build/temp_efi_esp.img"

    # Step 1: Create FAT32 ESP image using FAT32Builder
    build_uefi_disk(temp_fat_img, efi_loader, kernel_elf)
    with open(temp_fat_img, 'rb') as f:
        fat_data = f.read()

    fat_size = len(fat_data)
    fat_512_sectors = fat_size // 512
    fat_iso_sectors = (fat_size + ISO_SECTOR_SIZE - 1) // ISO_SECTOR_SIZE

    # Layout of ISO:
    # LBA 0..15: System area (16 * 2048 = 32KB)
    # LBA 16: Primary Volume Descriptor (PVD)
    # LBA 17: El Torito BRVD
    # LBA 18: Terminator
    # LBA 19: Boot Catalog
    # LBA 20..20+fat_iso_sectors-1: FAT ESP Image (efi.img)

    system_sectors = 16
    pvd_lba = 16
    brvd_lba = 17
    term_lba = 18
    catalog_lba = 19
    fat_lba = 20
    total_iso_sectors = fat_lba + fat_iso_sectors

    pvd, brvd, term = create_iso_descriptor_set(total_iso_sectors, catalog_lba)
    catalog = make_el_torito_catalog(fat_lba, fat_512_sectors)

    os.makedirs(os.path.dirname(os.path.abspath(output_iso)), exist_ok=True)
    with open(output_iso, 'wb') as iso:
        # Write 16 reserved system sectors (32 KB)
        iso.write(b'\x00' * (system_sectors * ISO_SECTOR_SIZE))

        # Write Descriptors
        iso.write(pvd)
        iso.write(brvd)
        iso.write(term)

        # Write Boot Catalog
        iso.write(catalog)

        # Write FAT ESP Data
        iso.write(fat_data)
        # Pad to full 2048 sector
        remainder = len(fat_data) % ISO_SECTOR_SIZE
        if remainder != 0:
            iso.write(b'\x00' * (ISO_SECTOR_SIZE - remainder))

    # Clean up temp image
    if os.path.exists(temp_fat_img):
        try:
            os.remove(temp_fat_img)
        except OSError:
            pass

    iso_size_mb = os.path.getsize(output_iso) / (1024 * 1024)
    print(f"[+] Bootable UEFI ISO generated successfully: {output_iso} ({iso_size_mb:.2f} MB)")

if __name__ == "__main__":
    out_iso = sys.argv[1] if len(sys.argv) > 1 else "build/auraos.iso"
    loader = sys.argv[2] if len(sys.argv) > 2 else "build/BOOTX64.EFI"
    kernel = sys.argv[3] if len(sys.argv) > 3 else "build/kernel.elf"
    build_uefi_iso(out_iso, loader, kernel)
