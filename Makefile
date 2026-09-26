# -----------------------------------------------------------------------------
# Xenithra OS v3.0 — Makefile
# Architecture: x86_64 | UEFI PE32+ Bootloader + Higher-Half Kernel ELF
# GUI: Node.js → Vite → Electron (replaces C compositor + Django)
# RTOS: 6-priority scheduler + DPC queue + priority inheritance
# Services: SysMain | DWM Proxy | MMCSS | AudioSrv | WMI
# IPC: kernel Named Pipe → ipc_bridge.cjs → Electron React Shell
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
LDFLAGS_KERN= -m elf_x86_64 -T $(KERN_DIR)/linker.ld -nostdlib

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
            $(BUILD_DIR)/sched_switch.o \
            $(BUILD_DIR)/kern_gdt_idt_asm.o \
            $(BUILD_DIR)/kern_ap_trampoline.o \
            $(BUILD_DIR)/kern_gdt_idt.o \
            $(BUILD_DIR)/kern_apic.o \
            $(BUILD_DIR)/kern_vmm.o \
            $(BUILD_DIR)/kern_heap.o \
            $(BUILD_DIR)/kern_kpcr.o \
            $(BUILD_DIR)/kern_smss.o \
            $(BUILD_DIR)/kern_kshell.o \
            $(BUILD_DIR)/kern_main.o \
            $(BUILD_DIR)/kern_ps2.o \
            $(BUILD_DIR)/kern_sound.o \
            $(BUILD_DIR)/kern_sched.o \
            $(BUILD_DIR)/kern_session.o \
            $(BUILD_DIR)/kern_firewall.o \
            $(BUILD_DIR)/kern_gui_ipc.o \
            $(BUILD_DIR)/svc_sysmain.o \
            $(BUILD_DIR)/svc_mmcss.o \
            $(BUILD_DIR)/svc_audiosrv.o \
            $(BUILD_DIR)/svc_wmi.o \
            $(BUILD_DIR)/svc_dwm_proxy.o \
            $(BUILD_DIR)/kern_app_browser.o \
            $(BUILD_DIR)/kern_app_explorer.o \
            $(BUILD_DIR)/kern_app_taskmgr.o \
            $(BUILD_DIR)/kern_app_firewall.o \
            $(BUILD_DIR)/kern_app_terminal.o \
            $(BUILD_DIR)/kern_app_vlc.o \
            $(BUILD_DIR)/kern_app_installer.o \
            $(BUILD_DIR)/kern_app_diskclone.o \
            $(BUILD_DIR)/kern_string.o \
            $(BUILD_DIR)/kern_font.o

.PHONY: all clean run run-iso setup_ovmf disk iso bootstrap shell shell-dev

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

$(BUILD_DIR)/sched_switch.o: $(KERN_DIR)/arch/x86_64/switch.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

# New Phase 0/1 kernel subsystem objects
$(BUILD_DIR)/kern_gdt_idt_asm.o: $(KERN_DIR)/arch/x86_64/gdt_idt.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kern_ap_trampoline.o: $(KERN_DIR)/arch/x86_64/ap_trampoline.asm | $(BUILD_DIR)
	$(AS) $(ASFLAGS) $< -o $@

$(BUILD_DIR)/kern_gdt_idt.o: $(KERN_DIR)/arch/x86_64/gdt_idt.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_apic.o: $(KERN_DIR)/arch/x86_64/apic.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_vmm.o: $(KERN_DIR)/mm/vmm.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_heap.o: $(KERN_DIR)/mm/heap.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_kpcr.o: $(KERN_DIR)/exec/kpcr.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_smss.o: $(KERN_DIR)/exec/smss.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_main.o: $(KERN_DIR)/main.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@


$(BUILD_DIR)/kern_ps2.o: $(KERN_DIR)/drivers/ps2.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_sound.o: $(KERN_DIR)/drivers/sound.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_sched.o: $(KERN_DIR)/sched/sched.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_session.o: $(KERN_DIR)/security/session.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_firewall.o: $(KERN_DIR)/security/firewall.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

# ── GUI IPC Server (replaces C compositor + Django) ─────────────────
$(BUILD_DIR)/kern_gui_ipc.o: $(KERN_DIR)/gui/gui_ipc.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

# ── Kernel Shell / GDB Stub ─────────────────────────────────────────
$(BUILD_DIR)/kern_kshell.o: $(KERN_DIR)/exec/kshell.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

# ── System Services ─────────────────────────────────────────────────
$(BUILD_DIR)/svc_sysmain.o: services/sysmain/sysmain.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -Iservices -c $< -o $@

$(BUILD_DIR)/svc_mmcss.o: services/mmcss/mmcss.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -Iservices -c $< -o $@

$(BUILD_DIR)/svc_audiosrv.o: services/audiosrv/audiosrv.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -Iservices -c $< -o $@

$(BUILD_DIR)/svc_wmi.o: services/wmi/wmi.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -Iservices -c $< -o $@

$(BUILD_DIR)/svc_dwm_proxy.o: services/dwm_proxy/dwm_proxy.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -Iservices -c $< -o $@

# ── Kernel Apps (draw-stripped IPC providers) ────────────────────────
$(BUILD_DIR)/kern_app_explorer.o: $(KERN_DIR)/apps/explorer_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_taskmgr.o: $(KERN_DIR)/apps/taskmgr_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_firewall.o: $(KERN_DIR)/apps/firewall_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_browser.o: $(KERN_DIR)/apps/browser_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_terminal.o: $(KERN_DIR)/apps/terminal_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_vlc.o: $(KERN_DIR)/apps/vlc_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_installer.o: $(KERN_DIR)/apps/installer_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_app_diskclone.o: $(KERN_DIR)/apps/diskclone_app.c | $(BUILD_DIR)
	$(CC_KERN) $(TARGET_KERN) -I$(SHARED_DIR) -I$(KERN_DIR) -c $< -o $@

$(BUILD_DIR)/kern_string.o: $(KERN_DIR)/kstring.c | $(BUILD_DIR)
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

# 8. Build Electron Desktop Shell (Node.js + Vite + React)
shell:
	cd desktop_shell && npm install && npm run build

shell-dev:
	cd desktop_shell && npm install && npm run dev &
	cd desktop_shell && sleep 3 && npm start

# 9. Launch QEMU Emulator
#    COM1 (0x3F8) = GUI IPC pipe → virtio-serial forwarded to host
#    COM2 (0x2F8) = GDB remote stub → TCP 1234
run: disk setup_ovmf
	$(QEMU) -bios $(OVMF) \
		-drive format=raw,file=$(DISK_IMG) \
		-m 4G \
		-smp 4 \
		-vga virtio \
		-serial pipe:$(BUILD_DIR)/xenithra_gui_ipc \
		-serial tcp::1234,server,nowait \
		-device virtio-serial \
		-chardev pipe,id=guipipe,path=$(BUILD_DIR)/xenithra_gui_ipc \
		-device virtconsole,chardev=guipipe \
		-no-reboot \
		-enable-kvm

run-iso: iso setup_ovmf
	$(QEMU) -bios $(OVMF) \
		-cdrom $(ISO_IMG) \
		-m 4G \
		-smp 4 \
		-vga virtio \
		-serial pipe:$(BUILD_DIR)/xenithra_gui_ipc \
		-serial tcp::1234,server,nowait \
		-no-reboot

run-debug: disk setup_ovmf
	$(QEMU) -bios $(OVMF) \
		-drive format=raw,file=$(DISK_IMG) \
		-m 4G \
		-smp 4 \
		-vga virtio \
		-serial stdio \
		-serial tcp::1234,server,nowait \
		-s -S \
		-no-reboot

clean:
	rm -rf $(BUILD_DIR)
