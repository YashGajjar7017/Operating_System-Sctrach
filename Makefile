# -----------------------------------------------------------------------------
# Xenithra OS - 64-bit Graphical Operating System Makefile
# Architecture: x86_64 | Target: UEFI PE32+ Application & Higher-Half Kernel ELF
# -----------------------------------------------------------------------------

BUILD_DIR = build
BOOT_DIR  = bootloader
KERN_DIR  = kernel
SHARED_DIR= shared
SCRIPTS_DIR = scripts

# Toolchain detection
CC_EFI    ?= clang
TARGET_EFI = -target x86_64-unknown-windows -ffreestanding -fshort-wchar -mno-red-zone -Wall -Wextra -O2
LDFLAGS_EFI= -nostdlib -Wl,-subsystem:efi_application -Wl,-entry:EfiMain

CC_KERN   ?= clang
TARGET_KERN= -target x86_64-unknown-none-elf -ffreestanding -mno-red-zone -mcmodel=kernel -Wall -Wextra -O2
LD_KERN   ?= ld.lld
LDFLAGS_KERN= -T $(KERN_DIR)/linker.ld -nostdlib

AS        ?= nasm
ASFLAGS   = -f elf64

PYTHON    ?= python3
QEMU      ?= qemu-system-x86_64
OVMF      ?= $(BUILD_DIR)/ovmf.fd

# Targets
BOOT_EFI  = $(BUILD_DIR)/BOOTX64.EFI
KERNEL_ELF= $(BUILD_DIR)/kernel.elf
DISK_IMG  = $(BUILD_DIR)/disk.img
ISO_IMG   = $(BUILD_DIR)/xenithra.iso

# Source Objects
BOOT_OBJS = $(BUILD_DIR)/boot_main.o \
            $(BUILD_DIR)/boot_gop.o \
            $(BUILD_DIR)/boot_ui.o \
            $(BUILD_DIR)/boot_font.o

KERN_OBJS = $(BUILD_DIR)/kern_entry.o \
            $(BUILD_DIR)/kern_main.o \
            $(BUILD_DIR)/kern_session.o \
            $(BUILD_DIR)/kern_firewall.o \
            $(BUILD_DIR)/kern_compositor.o \
            $(BUILD_DIR)/kern_dom.o \
            $(BUILD_DIR)/kern_app_firewall.o \
            $(BUILD_DIR)/kern_app_terminal.o \
            $(BUILD_DIR)/kern_font.o

.PHONY: all clean run run-iso setup_ovmf disk iso bootstrap

all: disk iso

# 1. Create build output directory
$(BUILD_DIR):
	mkdir -p $(BUILD_DIR)

# 2. Compile Bootloader Objects
$(BUILD_DIR)/boot_main.o: $(BOOT_DIR)/main.c | $(BUILD_DIR)
	$(CC_EFI) $(TARGET_EFI) -I$(SHARED_DIR) -I$(BOOT_DIR) -c $< -o $@

$(BUILD_DIR)/boot_gop.o: $(BOOT_DIR)/gop.c | $(BUILD_DIR)
	$(CC_EFI) $(TARGET_EFI) -I$(SHARED_DIR) -I$(BOOT_DIR) -c $< -o $@

$(BUILD_DIR)/boot_ui.o: $(BOOT_DIR)/ui.c | $(BUILD_DIR)
	$(CC_EFI) $(TARGET_EFI) -I$(SHARED_DIR) -I$(BOOT_DIR) -c $< -o $@

$(BUILD_DIR)/boot_font.o: $(SHARED_DIR)/font.c | $(BUILD_DIR)
	$(CC_EFI) $(TARGET_EFI) -I$(SHARED_DIR) -c $< -o $@

# 3. Link BOOTX64.EFI
$(BOOT_EFI): $(BOOT_OBJS)
	$(CC_EFI) $(TARGET_EFI) $(LDFLAGS_EFI) $(BOOT_OBJS) -o $@

# 4. Assemble & Compile Kernel Objects
$(BUILD_DIR)/kern_entry.o: $(KERN_DIR)/arch/x86_64/entry.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kern_main.o: $(KERN_DIR)/main.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_session.o: $(KERN_DIR)/security/session.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_firewall.o: $(KERN_DIR)/security/firewall.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_compositor.o: $(KERN_DIR)/gui/compositor.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_dom.o: $(KERN_DIR)/gui/dom_engine.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_firewall.o: $(KERN_DIR)/apps/firewall_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_terminal.o: $(KERN_DIR)/apps/terminal_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_font.o: $(SHARED_DIR)/font.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -c $< -o $@

# 5. Link Kernel ELF
$(KERNEL_ELF): $(KERN_OBJS) $(KERN_DIR)/linker.ld
	$(LD_KERN) $(LDFLAGS_KERN) $(KERN_OBJS) -o $@

# 6. Generate FAT32 UEFI ESP Disk Image
$(DISK_IMG): $(BOOT_EFI) $(KERNEL_ELF)
	$(PYTHON) $(SCRIPTS_DIR)/build_disk.py $(DISK_IMG) $(BOOT_EFI) $(KERNEL_ELF)

disk: $(DISK_IMG)

# 7. Generate Bootable UEFI ISO Image
$(ISO_IMG): $(BOOT_EFI) $(KERNEL_ELF)
	$(PYTHON) $(SCRIPTS_DIR)/build_iso.py $(ISO_IMG) $(BOOT_EFI) $(KERNEL_ELF)

iso: $(ISO_IMG)

bootstrap:
	$(PYTHON) $(SCRIPTS_DIR)/bootstrap_tools.py

setup_ovmf:
	$(PYTHON) $(SCRIPTS_DIR)/download_ovmf.py

# 8. Launch QEMU Emulator (Disk or CD-ROM ISO)
run: disk setup_ovmf
	$(QEMU) -bios $(OVMF) \
		-drive format=raw,file=$(DISK_IMG) \
		-m 2G \
		-vga std \
		-serial stdio \
		-no-reboot

run-iso: iso setup_ovmf
	$(QEMU) -bios $(OVMF) \
		-cdrom $(ISO_IMG) \
		-m 2G \
		-vga std \
		-serial stdio \
		-no-reboot

clean:
	rm -rf $(BUILD_DIR)
