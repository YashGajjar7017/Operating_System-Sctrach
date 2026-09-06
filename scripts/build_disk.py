#!/usr/bin/env python3
"""
@file build_disk.py
@brief Pure Python UEFI FAT32 ESP Disk Image Generator.
Creates a bootable UEFI FAT32 disk image without requiring root or external tools (mtools/mkfs).
"""

import os
import sys
import struct
import shutil

SECTOR_SIZE = 512
DISK_SIZE_MB = 64
TOTAL_SECTORS = (DISK_SIZE_MB * 1024 * 1024) // SECTOR_SIZE
SECTORS_PER_CLUSTER = 8  # 4KB clusters
RESERVED_SECTORS = 32
NUM_FATS = 2

class FAT32Builder:
    def __init__(self, filename, total_sectors=TOTAL_SECTORS):
        self.filename = filename
        self.total_sectors = total_sectors
        self.sectors_per_cluster = SECTORS_PER_CLUSTER
        self.cluster_size = self.sectors_per_cluster * SECTOR_SIZE
        self.reserved_sectors = RESERVED_SECTORS
        self.num_fats = NUM_FATS

        # Calculate FAT size
        total_clusters_approx = (self.total_sectors - self.reserved_sectors) // self.sectors_per_cluster
        fat_bytes_approx = total_clusters_approx * 4
        self.sectors_per_fat = (fat_bytes_approx + SECTOR_SIZE - 1) // SECTOR_SIZE

        # Recalculate cluster area
        self.data_start_sector = self.reserved_sectors + (self.num_fats * self.sectors_per_fat)
        self.total_clusters = (self.total_sectors - self.data_start_sector) // self.sectors_per_cluster

        self.root_cluster = 2
        self.fat = [0] * (self.total_clusters + 2)
        self.fat[0] = 0x0FFFFFF8  # Media descriptor
        self.fat[1] = 0x0FFFFFFF  # EOC marker
        self.fat[2] = 0x0FFFFFFF  # Root directory EOF

        self.next_free_cluster = 3
        self.cluster_data = {}  # cluster_index -> bytes (4096 bytes)

    def _get_cluster_bytes(self, cluster_idx):
        if cluster_idx not in self.cluster_data:
            self.cluster_data[cluster_idx] = bytearray(self.cluster_size)
        return self.cluster_data[cluster_idx]

    def allocate_cluster(self):
        c = self.next_free_cluster
        self.next_free_cluster += 1
        self.fat[c] = 0x0FFFFFFF  # EOF
        return c

    def write_file(self, data: bytes):
        """Writes data into a chain of clusters and returns the starting cluster."""
        if len(data) == 0:
            return 0

        first_cluster = self.allocate_cluster()
        cur_cluster = first_cluster
        offset = 0

        while offset < len(data):
            chunk = data[offset:offset + self.cluster_size]
            c_buf = self._get_cluster_bytes(cur_cluster)
            c_buf[:len(chunk)] = chunk
            offset += len(chunk)

            if offset < len(data):
                next_c = self.allocate_cluster()
                self.fat[cur_cluster] = next_c
                cur_cluster = next_c
            else:
                self.fat[cur_cluster] = 0x0FFFFFFF

        return first_cluster

    def create_dir_entry(self, name83: str, attr: int, first_cluster: int, size: int):
        entry = bytearray(32)
        # Format name to 8.3
        parts = name83.upper().split('.')
        base = parts[0][:8].ljust(8)
        ext = parts[1][:3].ljust(3) if len(parts) > 1 else "   "
        short_name = (base + ext).encode('ascii')

        entry[0:11] = short_name
        entry[11] = attr
        entry[20:22] = struct.pack('<H', (first_cluster >> 16) & 0xFFFF)  # High cluster
        entry[26:28] = struct.pack('<H', first_cluster & 0xFFFF)          # Low cluster
        entry[28:32] = struct.pack('<I', size)                            # File size
        return entry

    def add_directory(self, parent_cluster: int, dir_name: str):
        new_dir_cluster = self.allocate_cluster()
        dir_buf = self._get_cluster_bytes(new_dir_cluster)
        
        # '.' entry
        dir_buf[0:32] = self.create_dir_entry(".", 0x10, new_dir_cluster, 0)
        # '..' entry
        dir_buf[32:64] = self.create_dir_entry("..", 0x10, parent_cluster, 0)

        # Add to parent
        self._add_entry_to_dir(parent_cluster, self.create_dir_entry(dir_name, 0x10, new_dir_cluster, 0))
        return new_dir_cluster

    def add_file(self, dir_cluster: int, file_name: str, file_data: bytes):
        file_cluster = self.write_file(file_data)
        entry = self.create_dir_entry(file_name, 0x20, file_cluster, len(file_data))
        self._add_entry_to_dir(dir_cluster, entry)

    def _add_entry_to_dir(self, dir_cluster: int, entry_bytes: bytes):
        cur_c = dir_cluster
        while True:
            buf = self._get_cluster_bytes(cur_c)
            for i in range(0, len(buf), 32):
                if buf[i] == 0x00 or buf[i] == 0xE5:
                    buf[i:i+32] = entry_bytes
                    return
            # Need next cluster for directory
            if self.fat[cur_c] == 0x0FFFFFFF:
                next_c = self.allocate_cluster()
                self.fat[cur_c] = next_c
                cur_c = next_c
            else:
                cur_c = self.fat[cur_c]

    def build_disk(self):
        print(f"[*] Formatting FAT32 UEFI ESP disk image ({DISK_SIZE_MB}MB)...")
        os.makedirs(os.path.dirname(os.path.abspath(self.filename)), exist_ok=True)
        
        with open(self.filename, 'wb') as f:
            # 1. Write Boot Sector (BPB)
            bpb = bytearray(SECTOR_SIZE)
            bpb[0:3] = b'\xEB\x58\x90'  # JMP short
            bpb[3:11] = b'MSWIN4.1'     # OEM ID
            struct.pack_into('<H', bpb, 11, SECTOR_SIZE)
            bpb[13] = self.sectors_per_cluster
            struct.pack_into('<H', bpb, 14, self.reserved_sectors)
            bpb[16] = self.num_fats
            struct.pack_into('<H', bpb, 17, 0) # Root entries (0 for FAT32)
            struct.pack_into('<H', bpb, 19, 0) # Small sectors
            bpb[21] = 0xF8                     # Media descriptor
            struct.pack_into('<H', bpb, 22, 0) # Sectors per FAT (16-bit)
            struct.pack_into('<H', bpb, 24, 63)# Sectors per track
            struct.pack_into('<H', bpb, 26, 255)# Number of heads
            struct.pack_into('<I', bpb, 28, 0) # Hidden sectors
            struct.pack_into('<I', bpb, 32, self.total_sectors) # Large sectors (32-bit)

            # FAT32 Extended BPB
            struct.pack_into('<I', bpb, 36, self.sectors_per_fat)
            struct.pack_into('<H', bpb, 40, 0) # Ext flags
            struct.pack_into('<H', bpb, 42, 0) # FS Version
            struct.pack_into('<I', bpb, 44, self.root_cluster) # Root cluster (2)
            struct.pack_into('<H', bpb, 48, 1) # FSInfo sector
            struct.pack_into('<H', bpb, 50, 6) # Backup boot sector
            bpb[64] = 0x80                     # Drive number
            bpb[66] = 0x29                     # Extended boot signature
            struct.pack_into('<I', bpb, 67, 0x12345678) # Volume serial number
            bpb[71:82] = b'AURA_ESP   '       # Volume label
            bpb[82:90] = b'FAT32   '           # System identifier
            bpb[510:512] = b'\x55\xAA'         # Signature

            f.write(bpb)

            # 2. Write FSInfo Sector
            fsinfo = bytearray(SECTOR_SIZE)
            fsinfo[0:4] = b'RRaA'              # Lead signature
            fsinfo[484:488] = b'rrAa'          # Struct signature
            struct.pack_into('<I', fsinfo, 488, self.total_clusters - self.next_free_cluster) # Free clusters
            struct.pack_into('<I', fsinfo, 492, self.next_free_cluster) # Next free cluster
            fsinfo[510:512] = b'\x55\xAA'
            f.write(fsinfo)

            # 3. Fill remaining reserved sectors
            for _ in range(self.reserved_sectors - 2):
                f.write(b'\x00' * SECTOR_SIZE)

            # 4. Write FAT tables
            fat_bytes = bytearray(self.sectors_per_fat * SECTOR_SIZE)
            for i, val in enumerate(self.fat):
                if i * 4 + 4 <= len(fat_bytes):
                    struct.pack_into('<I', fat_bytes, i * 4, val & 0x0FFFFFFF)

            for _ in range(self.num_fats):
                f.write(fat_bytes)

            # 5. Write Data Clusters
            for c in range(2, self.next_free_cluster):
                if c in self.cluster_data:
                    f.write(self.cluster_data[c])
                else:
                    f.write(b'\x00' * self.cluster_size)

            # Pad remaining disk sectors
            written_bytes = f.tell()
            target_bytes = self.total_sectors * SECTOR_SIZE
            if written_bytes < target_bytes:
                f.write(b'\x00' * (target_bytes - written_bytes))

        print(f"[+] Successfully generated UEFI FAT32 ESP Disk Image: {self.filename} ({os.path.getsize(self.filename)} bytes)")

def build_uefi_disk(output_img, efi_loader, kernel_elf):
    fat = FAT32Builder(output_img)

    # 1. Create \EFI and \EFI\BOOT
    efi_cluster = fat.add_directory(fat.root_cluster, "EFI")
    boot_cluster = fat.add_directory(efi_cluster, "BOOT")
    aura_cluster = fat.add_directory(fat.root_cluster, "AURAOS")

    # 2. Add BOOTX64.EFI
    if os.path.exists(efi_loader):
        with open(efi_loader, 'rb') as f:
            efi_data = f.read()
        fat.add_file(boot_cluster, "BOOTX64.EFI", efi_data)
        print(f"[+] Added \\EFI\\BOOT\\BOOTX64.EFI ({len(efi_data)} bytes)")
    else:
        print(f"[!] Warning: {efi_loader} not found. Skipping.")

    # 3. Add KERNEL.ELF
    if os.path.exists(kernel_elf):
        with open(kernel_elf, 'rb') as f:
            kernel_data = f.read()
        fat.add_file(fat.root_cluster, "KERNEL.ELF", kernel_data)
        fat.add_file(aura_cluster, "KERNEL.ELF", kernel_data)
        print(f"[+] Added \\KERNEL.ELF and \\AURAOS\\KERNEL.ELF ({len(kernel_data)} bytes)")
    else:
        print(f"[!] Warning: {kernel_elf} not found. Skipping.")

    fat.build_disk()

if __name__ == "__main__":
    out = sys.argv[1] if len(sys.argv) > 1 else "build/disk.img"
    loader = sys.argv[2] if len(sys.argv) > 2 else "build/BOOTX64.EFI"
    kernel = sys.argv[3] if len(sys.argv) > 3 else "build/kernel.elf"
    build_uefi_disk(out, loader, kernel)
