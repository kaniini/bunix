ARCH := x86_64
BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso
ESP_DIR := $(BUILD_DIR)/esp
EFI_BOOT_IMG := $(BUILD_DIR)/bunixos-efi.iso
EFI_BOOT_APP := $(ESP_DIR)/EFI/BOOT/BOOTX64.EFI
KERNEL := $(BUILD_DIR)/bunixos.kernel
HELLO_MODULE := $(BUILD_DIR)/modules/hello.server
HELLO_MODULE_OBJ := $(BUILD_DIR)/user/hello/start.S.o

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
	arch/$(ARCH)/interrupts.c \
	arch/$(ARCH)/interrupts.S \
	arch/$(ARCH)/thread.c \
	arch/$(ARCH)/thread.S \
	arch/$(ARCH)/syscall.S \
	arch/$(ARCH)/user.c \
	arch/$(ARCH)/vm.c \
	kernel/main.c \
	kernel/console.c \
	kernel/elf.c \
	kernel/ipc.c \
	kernel/multiboot2.c \
	kernel/pmm.c \
	kernel/sched.c \
	kernel/server.c \
	kernel/timer.c \
	kernel/vm.c \
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

$(HELLO_MODULE_OBJ): user/hello/start.S
	mkdir -p $(dir $@)
	$(CC) -m64 -ffreestanding -fno-pic -fno-pie -c $< -o $@

$(HELLO_MODULE): $(HELLO_MODULE_OBJ) user/user.ld
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(HELLO_MODULE_OBJ)

$(EFI_BOOT_APP): $(KERNEL) boot/grub-standalone.cfg $(HELLO_MODULE) modules/ping.server modules/vm.server
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(ESP_DIR)/EFI/BOOT $(ESP_DIR)/boot
	cp $(KERNEL) $(ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=boot/grub-standalone.cfg" \
		"boot/bunixos.kernel=$(KERNEL)" \
		"modules/hello.server=$(HELLO_MODULE)" \
		"modules/ping.server=modules/ping.server" \
		"modules/vm.server=modules/vm.server"

$(EFI_BOOT_IMG): $(KERNEL) boot/grub.cfg $(HELLO_MODULE) modules/ping.server modules/vm.server
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
	cp $(HELLO_MODULE) $(ISO_ROOT)/modules/hello.server
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
	grep -F "interrupts: idt loaded" $(BUILD_DIR)/serial.log
	grep -F "timer: pit 100hz" $(BUILD_DIR)/serial.log
	grep -F "interrupts: enabled" $(BUILD_DIR)/serial.log
	grep -F "timer: tick 1" $(BUILD_DIR)/serial.log
	grep -F "user: gdt/tss/syscall ready" $(BUILD_DIR)/serial.log
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
	grep -F "ipc: port create vm" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv block port=vm" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=vm type=2 sender=0" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv port=vm type=2 sender=0" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=kernel-rpc type=2 sender=1" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv port=kernel-rpc type=2 sender=1" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=vm type=1 sender=3" $(BUILD_DIR)/serial.log
	grep -F "sched: preemption enabled" $(BUILD_DIR)/serial.log
	grep -F "sched: preempt tid=3 cpu=0" $(BUILD_DIR)/serial.log
	grep -F "vm-server: ipc event type=1 sender=3 word0=0x2a" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server hello" $(BUILD_DIR)/serial.log
	grep -F "elf: entry=0x0000000000400000" $(BUILD_DIR)/serial.log
	grep -F "user: enter rip=0x0000000000400000" $(BUILD_DIR)/serial.log
	grep -F "hello: world <3" $(BUILD_DIR)/serial.log
	grep -F "syscall: exit status=0" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server ping" $(BUILD_DIR)/serial.log
	grep -F "ping: one" $(BUILD_DIR)/serial.log
	grep -F "ping: two" $(BUILD_DIR)/serial.log
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
