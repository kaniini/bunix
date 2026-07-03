ARCH := x86_64
BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso
ESP_DIR := $(BUILD_DIR)/esp
EFI_BOOT_IMG := $(BUILD_DIR)/bunixos-efi.iso
EFI_BOOT_APP := $(ESP_DIR)/EFI/BOOT/BOOTX64.EFI
KERNEL := $(BUILD_DIR)/bunixos.kernel
INIT_MODULE := $(BUILD_DIR)/modules/init.server
USER_CRT0_OBJ := $(BUILD_DIR)/user/crt0.S.o
INIT_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/init/main.c.o
NAMES_MODULE := $(BUILD_DIR)/modules/names.server
NAMES_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/names/main.c.o
BLOCK_MODULE := $(BUILD_DIR)/modules/block.server
BLOCK_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/block/main.c.o
VFS_MODULE := $(BUILD_DIR)/modules/vfs.server
VFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/vfs/main.c.o
HELLO_MODULE := $(BUILD_DIR)/modules/hello.server
HELLO_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/hello/main.c.o
PING_MODULE := $(BUILD_DIR)/modules/ping.server
PING_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/ping/main.c.o

CC ?= gcc
LD ?= ld
OBJDUMP ?= objdump
QEMU ?= qemu-system-x86_64
SMP ?= 2
GRUB_MKRESCUE ?= grub-mkrescue
GRUB_MKSTANDALONE ?= grub-mkstandalone
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE.fd

CFLAGS := -m64 -std=c11 -O2 -g -ffreestanding -fno-stack-protector \
	-fno-pic -fno-pie -fno-builtin -mno-red-zone \
	-mno-sse -mno-sse2 \
	-Iarch/$(ARCH)/include -Ikernel \
	-Wall -Wextra -Werror -MMD -MP
ASFLAGS := -m64 -g -ffreestanding -Iarch/$(ARCH)/include -Ikernel -MMD -MP
LDFLAGS := -m elf_x86_64 -nostdlib -T linker.ld
USER_CFLAGS := -m64 -std=c11 -O2 -g -ffreestanding -fno-stack-protector \
	-fno-pic -fno-pie -fno-builtin -mno-red-zone \
	-mno-sse -mno-sse2 \
	-Iuser/include -Wall -Wextra -Werror -MMD -MP
USER_ASFLAGS := -m64 -g -ffreestanding -fno-pic -fno-pie \
	-Iuser/include -MMD -MP

KERNEL_SRCS := \
	arch/$(ARCH)/boot/multiboot2.S \
	arch/$(ARCH)/ap_trampoline.S \
	arch/$(ARCH)/interrupts.c \
	arch/$(ARCH)/smp.c \
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
	kernel/name.c \
	kernel/pmm.c \
	kernel/sched.c \
	kernel/server.c \
	kernel/spinlock.c \
	kernel/timer.c \
	kernel/vm.c \
	servers/vm/vm.c

KERNEL_OBJS := $(KERNEL_SRCS:%=$(BUILD_DIR)/%.o)
USER_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/init/main.c.o \
	$(BUILD_DIR)/user/names/main.c.o \
	$(BUILD_DIR)/user/block/main.c.o \
	$(BUILD_DIR)/user/vfs/main.c.o \
	$(BUILD_DIR)/user/hello/main.c.o $(BUILD_DIR)/user/ping/main.c.o
DEPS := $(KERNEL_OBJS:.o=.d) $(USER_OBJS:.o=.d)

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

$(BUILD_DIR)/user/%.c.o: user/%.c
	mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/%.S.o: user/%.S
	mkdir -p $(dir $@)
	$(CC) $(USER_ASFLAGS) -c $< -o $@

$(INIT_MODULE): $(INIT_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(INIT_MODULE_OBJS)

$(NAMES_MODULE): $(NAMES_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(NAMES_MODULE_OBJS)

$(BLOCK_MODULE): $(BLOCK_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(BLOCK_MODULE_OBJS)

$(VFS_MODULE): $(VFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(VFS_MODULE_OBJS)

$(HELLO_MODULE): $(HELLO_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(HELLO_MODULE_OBJS)

$(PING_MODULE): $(PING_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(PING_MODULE_OBJS)

$(EFI_BOOT_APP): $(KERNEL) boot/grub-standalone.cfg $(INIT_MODULE) $(NAMES_MODULE) $(BLOCK_MODULE) $(VFS_MODULE) $(HELLO_MODULE) $(PING_MODULE) modules/vm.server
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(ESP_DIR)/EFI/BOOT $(ESP_DIR)/boot
	cp $(KERNEL) $(ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=boot/grub-standalone.cfg" \
		"boot/bunixos.kernel=$(KERNEL)" \
		"modules/names.server=$(NAMES_MODULE)" \
		"modules/init.server=$(INIT_MODULE)" \
		"modules/block.server=$(BLOCK_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/hello.server=$(HELLO_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/vm.server=modules/vm.server"

$(EFI_BOOT_IMG): $(KERNEL) boot/grub.cfg $(INIT_MODULE) $(NAMES_MODULE) $(BLOCK_MODULE) $(VFS_MODULE) $(HELLO_MODULE) $(PING_MODULE) modules/vm.server
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
	cp $(NAMES_MODULE) $(ISO_ROOT)/modules/names.server
	cp $(INIT_MODULE) $(ISO_ROOT)/modules/init.server
	cp $(BLOCK_MODULE) $(ISO_ROOT)/modules/block.server
	cp $(VFS_MODULE) $(ISO_ROOT)/modules/vfs.server
	cp $(HELLO_MODULE) $(ISO_ROOT)/modules/hello.server
	cp $(PING_MODULE) $(ISO_ROOT)/modules/ping.server
	cp modules/vm.server $(ISO_ROOT)/modules/vm.server
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

run: $(EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		-serial stdio -display none -no-reboot

run-kernel: $(EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		-serial stdio -display none -no-reboot

run-iso: $(EFI_BOOT_IMG)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-cdrom $(EFI_BOOT_IMG) -serial stdio -display none -no-reboot

test: $(EFI_BOOT_APP)
	mkdir -p $(BUILD_DIR)
	truncate -s 0 $(BUILD_DIR)/serial.log
	timeout 10s $(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
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
	grep -F "multiboot2: acpi rsdp=" $(BUILD_DIR)/serial.log
	grep -F "smp: discovered cpus=2" $(BUILD_DIR)/serial.log
	grep -F "timer: lapic cpu=0 periodic" $(BUILD_DIR)/serial.log
	grep -F "timer: lapic cpu=1 periodic" $(BUILD_DIR)/serial.log
	grep -F "smp: ap online cpu=1" $(BUILD_DIR)/serial.log
	grep -F "smp: started aps=1" $(BUILD_DIR)/serial.log
	grep -F "sched: init cpus=2 boot_cpu=0" $(BUILD_DIR)/serial.log
	grep -F "sched: cpu=1 online" $(BUILD_DIR)/serial.log
	grep -F "smp: scheduler aps=1" $(BUILD_DIR)/serial.log
	grep -F "sched: ipi cpu=1" $(BUILD_DIR)/serial.log
	grep -F "names: init entries=32" $(BUILD_DIR)/serial.log
	grep -F "names: register name=vm id=1 kind=2" $(BUILD_DIR)/serial.log
	grep -F "names: register name=console id=2 kind=2" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=vm id=1" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=names id=2" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=init id=3" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=block id=4" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=vfs id=5" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=hello id=6" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=ping id=7" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=1 name=vm vm=1" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=1 task=1 name=vm" $(BUILD_DIR)/serial.log
	grep -F "sched: place tid=1 cpu=0 policy=auto" $(BUILD_DIR)/serial.log
	grep -F "sched: enqueue tid=1 cpu=0" $(BUILD_DIR)/serial.log
	grep -F "sched: switch cpu=0 prev=0 next=1" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=2 name=names vm=2" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=2 task=2 name=names" $(BUILD_DIR)/serial.log
	grep -F "sched: place tid=2 cpu=1 policy=auto" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=3 name=init vm=3" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=3 task=3 name=init" $(BUILD_DIR)/serial.log
	grep -F "sched: place tid=3 cpu=0 policy=auto" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=4 name=block vm=4" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=4 task=4 name=block" $(BUILD_DIR)/serial.log
	grep -F "sched: place tid=4 cpu=1 policy=auto" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=5 name=vfs vm=5" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=5 task=5 name=vfs" $(BUILD_DIR)/serial.log
	grep -F "sched: place tid=5 cpu=0 policy=auto" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=6 name=hello vm=6" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=6 task=6 name=hello" $(BUILD_DIR)/serial.log
	grep -F "sched: task pid=7 name=ping vm=7" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=7 task=7 name=ping" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server vm" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server names" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server init" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server block" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server vfs" $(BUILD_DIR)/serial.log
	grep -F "vm-server: memory authority online" $(BUILD_DIR)/serial.log
	grep -F "vm-server: rpc free_frame" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create vm" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create console" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create names" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create init" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create block" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create vfs" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create hello" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create ping" $(BUILD_DIR)/serial.log
	grep -F "ipc: port create reply" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=1 handle=1 type=port rights=0x7" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=2 handle=1 type=port rights=0x7" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=2 handle=2 type=port rights=0x5" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=3 handle=1 type=port rights=0x7" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=3 handle=2 type=port rights=0x5" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=3 handle=3 type=port rights=0x5" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=3 handle=4 type=port rights=0x5" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=2 handle=3 type=port rights=0x1" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=2 handle=4 type=port rights=0x5" $(BUILD_DIR)/serial.log
	grep -F "sched: inherit denied task=3 handle=2 requested=0x3 rights=0x5" $(BUILD_DIR)/serial.log
	grep -F "kernel: invalid inherited cap handle=2 rights=0x3 for hello" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=4 handle=1 type=port rights=0x7" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=4 handle=2 type=port rights=0x1" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=5 handle=1 type=port rights=0x7" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=5 handle=2 type=port rights=0x1" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=5 handle=3 type=port rights=0x1" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=6 handle=1 type=port rights=0x7" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=6 handle=2 type=port rights=0x1" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=7 handle=1 type=port rights=0x7" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=7 handle=2 type=port rights=0x1" $(BUILD_DIR)/serial.log
	grep -F "sched: grant task=7 handle=3 type=port rights=0x1" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv block port=vm" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv block port=names" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv block port=block" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv block port=vfs" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv block port=reply" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=names proto=0x454d414e type=1 sender=3 queued=1" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv port=names proto=0x454d414e type=1 sender=3 queued=0" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=names proto=0x454d414e type=2 sender=3 queued=1" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv port=names proto=0x454d414e type=2 sender=3 queued=0" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=vfs proto=0x30534656 type=1 sender=3 queued=1" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv port=vfs proto=0x30534656 type=1 sender=3 queued=0" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=block proto=0x304b4c42 type=2 sender=5 queued=1" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv port=block proto=0x304b4c42 type=2 sender=5 queued=0" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=ping proto=0x474e4950 type=1 sender=3 queued=1" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv port=ping proto=0x474e4950 type=1 sender=3 queued=0" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=vm proto=0x4d454d56 type=1 sender=7" $(BUILD_DIR)/serial.log
	grep -F "sched: close task=7 handle=3 type=port rights=0x1" $(BUILD_DIR)/serial.log
	grep -F "sched: handle denied task=7 handle=3 need=0x1 rights=0x0" $(BUILD_DIR)/serial.log
	grep -F "ipc: send port=reply proto=0x474e4950 type=2 sender=7 queued=1" $(BUILD_DIR)/serial.log
	grep -F "ipc: recv port=reply proto=0x474e4950 type=2 sender=7 queued=0" $(BUILD_DIR)/serial.log
	grep -F "sched: preemption enabled" $(BUILD_DIR)/serial.log
	grep -F "vm-server: ipc event proto=0x4d454d56 type=1 sender=7 word0=0x2a" $(BUILD_DIR)/serial.log
	grep -F "names: online" $(BUILD_DIR)/serial.log
	grep -F "names: registered" $(BUILD_DIR)/serial.log
	grep -F "names: resolved" $(BUILD_DIR)/serial.log
	grep -F "block: online" $(BUILD_DIR)/serial.log
	grep -F "vfs: online" $(BUILD_DIR)/serial.log
	grep -F "vfs: mounted block" $(BUILD_DIR)/serial.log
	grep -F "init: launching servers" $(BUILD_DIR)/serial.log
	grep -F "init: names ready" $(BUILD_DIR)/serial.log
	grep -F "init: fs ready" $(BUILD_DIR)/serial.log
	grep -F "rootfs: hello" $(BUILD_DIR)/serial.log
	grep -F "init: bad cap denied" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server block" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server vfs" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server hello" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server ping" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server hello" $(BUILD_DIR)/serial.log
	grep -F "elf: entry=0x0000000000400000" $(BUILD_DIR)/serial.log
	grep -F "user: enter rip=0x0000000000400000" $(BUILD_DIR)/serial.log
	grep -F "hello: world <3" $(BUILD_DIR)/serial.log
	grep -F "hello: vm denied" $(BUILD_DIR)/serial.log
	grep -F "sched: handle denied task=6 handle=3 need=0x1 rights=0x0" $(BUILD_DIR)/serial.log
	grep -F "syscall: exit status=0" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server ping" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server ping image=0x" $(BUILD_DIR)/serial.log
	grep -F "elf: load vaddr=0x0000000000400000" $(BUILD_DIR)/serial.log
	grep -F "ping: one" $(BUILD_DIR)/serial.log
	grep -F "ping: vm closed" $(BUILD_DIR)/serial.log
	grep -F "ping: two" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=3 exited" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=6 exited" $(BUILD_DIR)/serial.log
	grep -F "sched: thread tid=7 exited" $(BUILD_DIR)/serial.log
	grep -F "sched: reap tid=3 task=3 name=init remaining=0" $(BUILD_DIR)/serial.log
	grep -F "sched: reap tid=6 task=6 name=hello remaining=0" $(BUILD_DIR)/serial.log
	grep -F "sched: reap tid=7 task=7 name=ping remaining=0" $(BUILD_DIR)/serial.log

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
