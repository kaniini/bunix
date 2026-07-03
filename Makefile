ARCH := x86_64
BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso
ESP_DIR := $(BUILD_DIR)/esp
EFI_BOOT_IMG := $(BUILD_DIR)/bunixos-efi.iso
EFI_BOOT_APP := $(ESP_DIR)/EFI/BOOT/BOOTX64.EFI
KERNEL := $(BUILD_DIR)/bunixos.kernel

CC ?= gcc
LD ?= ld
OBJDUMP ?= objdump
QEMU ?= qemu-system-x86_64
GRUB_MKRESCUE ?= grub-mkrescue
GRUB_MKSTANDALONE ?= grub-mkstandalone
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE.fd

CFLAGS := -m64 -std=c11 -O2 -g -ffreestanding -fno-stack-protector \
	-fno-pic -fno-pie -fno-builtin -mno-red-zone \
	-Iarch/$(ARCH)/include -Ikernel \
	-Wall -Wextra -Werror -MMD -MP
ASFLAGS := -m64 -g -ffreestanding -Iarch/$(ARCH)/include -Ikernel -MMD -MP
LDFLAGS := -m elf_x86_64 -nostdlib -T linker.ld

KERNEL_SRCS := \
	arch/$(ARCH)/boot/multiboot2.S \
	arch/$(ARCH)/thread.c \
	arch/$(ARCH)/thread.S \
	arch/$(ARCH)/vm.c \
	kernel/main.c \
	kernel/console.c \
	kernel/multiboot2.c \
	kernel/pmm.c \
	kernel/sched.c \
	kernel/server.c \
	kernel/vm.c \
	servers/hello/hello.c \
	servers/ping/ping.c \
	servers/vm/vm.c

KERNEL_OBJS := $(KERNEL_SRCS:%=$(BUILD_DIR)/%.o)
DEPS := $(KERNEL_OBJS:.o=.d)

.PHONY: all clean run run-kernel run-iso test iso esp check-tools

all: $(KERNEL)

$(KERNEL): $(KERNEL_OBJS) linker.ld
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)
	$(OBJDUMP) -h $@ | awk '/multiboot/ { if (strtonum("0x" $$6) >= 0x8000) exit 1 }'

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

iso: $(EFI_BOOT_IMG)

esp: $(EFI_BOOT_APP)

$(EFI_BOOT_APP): $(KERNEL) boot/grub-standalone.cfg modules/hello.server modules/ping.server modules/vm.server
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(ESP_DIR)/EFI/BOOT $(ESP_DIR)/boot
	cp $(KERNEL) $(ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=boot/grub-standalone.cfg" \
		"boot/bunixos.kernel=$(KERNEL)" \
		"modules/hello.server=modules/hello.server" \
		"modules/ping.server=modules/ping.server" \
		"modules/vm.server=modules/vm.server"

$(EFI_BOOT_IMG): $(KERNEL) boot/grub.cfg modules/hello.server modules/ping.server modules/vm.server
	@if ! command -v $(GRUB_MKRESCUE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKRESCUE)"; exit 1; \
	fi
	@if ! command -v xorriso >/dev/null 2>&1; then \
		echo "missing xorriso, required by grub-mkrescue"; exit 1; \
	fi
	mkdir -p $(ISO_ROOT)/boot/grub
	mkdir -p $(ISO_ROOT)/modules
	cp $(KERNEL) $(ISO_ROOT)/boot/bunixos.kernel
	cp boot/grub.cfg $(ISO_ROOT)/boot/grub/grub.cfg
	cp modules/hello.server $(ISO_ROOT)/modules/hello.server
	cp modules/ping.server $(ISO_ROOT)/modules/ping.server
	cp modules/vm.server $(ISO_ROOT)/modules/vm.server
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

run: $(EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		-serial stdio -display none -no-reboot

run-kernel: $(EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		-serial stdio -display none -no-reboot

run-iso: $(EFI_BOOT_IMG)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-cdrom $(EFI_BOOT_IMG) -serial stdio -display none -no-reboot

test: $(EFI_BOOT_APP)
	mkdir -p $(BUILD_DIR)
	truncate -s 0 $(BUILD_DIR)/serial.log
	timeout 10s $(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		-serial file:$(BUILD_DIR)/serial.log -display none -no-reboot; \
		status=$$?; test $$status -eq 0 -o $$status -eq 124
	grep -F "multiboot2: module" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=vm id=1" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=hello id=2" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=ping id=3" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=1 name=vm vm=1" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=1 task=1 name=vm" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=2 name=hello vm=2" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=2 task=2 name=hello" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=3 name=ping vm=3" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=3 task=3 name=ping" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server vm" $(BUILD_DIR)/serial.log
	grep -F "vm-server: memory authority online" $(BUILD_DIR)/serial.log
	grep -F "vm-server: rpc free_frame" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server hello" $(BUILD_DIR)/serial.log
	grep -F "hello: world <3" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server ping" $(BUILD_DIR)/serial.log
	grep -F "ping: one" $(BUILD_DIR)/serial.log
	grep -F "sched: yield tid=3 cpu=0" $(BUILD_DIR)/serial.log
	grep -F "ping: two" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=1 exited" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=2 exited" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=3 exited" $(BUILD_DIR)/serial.log

check-tools:
	@command -v $(CC)
	@command -v $(LD)
	@command -v $(QEMU)
	@command -v $(GRUB_MKRESCUE)
	@command -v $(GRUB_MKSTANDALONE)
	@test -r $(OVMF_CODE)

clean:
	rm -rf $(BUILD_DIR)

-include $(DEPS)
