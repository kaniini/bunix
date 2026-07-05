ARCH := x86_64
BUILD_DIR := build
ISO_ROOT := $(BUILD_DIR)/iso
ESP_DIR := $(BUILD_DIR)/esp
EFI_BOOT_IMG := $(BUILD_DIR)/bunixos-efi.iso
EFI_BOOT_APP := $(ESP_DIR)/EFI/BOOT/BOOTX64.EFI
KERNEL := $(BUILD_DIR)/bunixos.kernel
BOOTSTRAP_MODULE := $(BUILD_DIR)/modules/bootstrap.server
USER_CRT0_OBJ := $(BUILD_DIR)/user/crt0.S.o
BOOTSTRAP_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/bootstrap/main.c.o
CONSOLE_MODULE := $(BUILD_DIR)/modules/console.server
CONSOLE_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/console/main.c.o
NAMES_MODULE := $(BUILD_DIR)/modules/names.server
NAMES_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/names/main.c.o
TIME_MODULE := $(BUILD_DIR)/modules/time.server
TIME_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/time/main.c.o
USER_MODULE := $(BUILD_DIR)/modules/user.server
USER_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/user/main.c.o
LINUX_SERVER_MODULE := $(BUILD_DIR)/modules/linux.server
LINUX_SERVER_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/linux/main.c.o
PROC_MODULE := $(BUILD_DIR)/modules/proc.server
PROC_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/proc/main.c.o
PROCFS_MODULE := $(BUILD_DIR)/modules/procfs.server
PROCFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/procfs/main.c.o
TMPFS_MODULE := $(BUILD_DIR)/modules/tmpfs.server
TMPFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/tmpfs/main.c.o
DEVFS_MODULE := $(BUILD_DIR)/modules/devfs.server
DEVFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/devfs/main.c.o
UTMPFS_MODULE := $(BUILD_DIR)/modules/utmpfs.server
UTMPFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/utmpfs/main.c.o
ROOTFS_MODULE := $(BUILD_DIR)/modules/rootfs.server
ROOTFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/rootfs/main.c.o
UNIONFS_MODULE := $(BUILD_DIR)/modules/unionfs.server
UNIONFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/unionfs/main.c.o
BLOCK_MODULE := $(BUILD_DIR)/modules/block.server
BLOCK_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/block/main.c.o
VFS_MODULE := $(BUILD_DIR)/modules/vfs.server
VFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/vfs/main.c.o
FIRST_MODULE := $(BUILD_DIR)/modules/first.user
FIRST_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/first/main.c.o
IPCSTRESS_MODULE := $(BUILD_DIR)/modules/ipcstress.user
IPCSTRESS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/ipcstress/main.c.o
LOGIN_MODULE := $(BUILD_DIR)/modules/login.user
LOGIN_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/login/main.c.o
LXTEST_MODULE := $(BUILD_DIR)/modules/lxtest.user
LXTEST_MODULE_OBJS := $(BUILD_DIR)/user/lxtest/main.S.o
GETDENTSTEST_MODULE := $(BUILD_DIR)/modules/getdentstest.user
GETDENTSTEST_MODULE_OBJS := $(BUILD_DIR)/user/getdentstest/main.S.o
EXECOK_MODULE := $(BUILD_DIR)/modules/execok.user
EXECOK_MODULE_OBJS := $(BUILD_DIR)/user/execok/main.S.o
MUSL_HELLO_MODULE := $(BUILD_DIR)/modules/musl-hello.user
FPUTEST_MODULE := $(BUILD_DIR)/modules/fputest.user
DYN_HELLO_MODULE := $(BUILD_DIR)/modules/dyn-hello.user
BUSYBOX_DYNAMIC ?= /bin/busybox
BUSYBOX_STATIC ?= /usr/bin/busybox.static
BUSYBOX ?= $(BUSYBOX_DYNAMIC)
MUSL_LDSO ?= /lib/ld-musl-x86_64.so.1
PING_MODULE := $(BUILD_DIR)/modules/ping.server
PING_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/ping/main.c.o
BLOCK_IMAGE := $(BUILD_DIR)/modules/disk0.img
ROOTFS_TOOL := $(BUILD_DIR)/tools/mkrootfs
ROOTFS_HELLO := modules/hello.txt
ROOTFS_SECRET := modules/secret.txt
ROOTFS_NESTED := modules/nested.txt
ROOTFS_LONG_PATH := /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/rootfs-entry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/hello.txt
ROOTFS_LONG_EXEC_PATH := /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/linux-execve-path-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/dyn-hello
ROOTFS_LONG_PROC_EXEC_PATH := /usr/share/bunix/alpine/very/long/rootfs/path/that/exceeds/the/old/two-hundred-fifty-six-byte/proc-exec-registry-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/path-component-that-forces-the-rootfs-image-format-past-the-old-limit/with-extra-components/first
ROOTFS_LONG_SYMLINK := /usr/share/bunix/long-target-link
ROOTFS_PASSWD := modules/passwd
ROOTFS_SHADOW := modules/shadow
ROOTFS_GROUP := modules/group
ROOTFS_INITTAB := modules/inittab
ROOTFS_EXECS := modules/execs
ROOTFS_SPAWNS := modules/spawns
ROOTFS_BUSYBOX_LINKS := \
	--symlink /sbin/init /bin/busybox \
	--symlink /bin/sh /bin/busybox \
	--symlink /usr/bin/env /bin/busybox \
	--symlink /bin/dmesg /bin/busybox \
	--symlink /bin/cat /bin/busybox \
	--symlink /bin/stat /bin/busybox \
	--symlink /bin/uptime /bin/busybox \
	--symlink /bin/sleep /bin/busybox \
	--symlink /bin/ls /bin/busybox \
	--symlink /bin/mount /bin/busybox \
	--symlink /bin/free /bin/busybox \
	--symlink /bin/ps /bin/busybox \
	--symlink /bin/top /bin/busybox \
	--symlink /bin/id /bin/busybox \
	--symlink /bin/kill /bin/busybox \
	--symlink /bin/echo /bin/busybox \
	--symlink /bin/env /bin/busybox \
	--symlink /bin/stty /bin/busybox \
	--symlink /bin/pwd /bin/busybox

CC ?= gcc
MUSL_CC ?= x86_64-alpine-linux-musl-gcc
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
	kernel/buffer.c \
	kernel/console.c \
	kernel/elf.c \
	kernel/ipc.c \
	kernel/multiboot2.c \
	kernel/name.c \
	kernel/pmm.c \
	kernel/sched.c \
	kernel/slab.c \
	kernel/server.c \
	kernel/spinlock.c \
	kernel/timer.c \
	kernel/vm.c \
	servers/vm/vm.c

KERNEL_OBJS := $(KERNEL_SRCS:%=$(BUILD_DIR)/%.o)
USER_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/bootstrap/main.c.o \
	$(BUILD_DIR)/user/console/main.c.o \
	$(BUILD_DIR)/user/names/main.c.o \
	$(BUILD_DIR)/user/time/main.c.o \
	$(BUILD_DIR)/user/user/main.c.o \
	$(BUILD_DIR)/user/linux/main.c.o \
	$(BUILD_DIR)/user/proc/main.c.o \
	$(BUILD_DIR)/user/procfs/main.c.o \
	$(BUILD_DIR)/user/tmpfs/main.c.o \
	$(BUILD_DIR)/user/devfs/main.c.o \
	$(BUILD_DIR)/user/utmpfs/main.c.o \
	$(BUILD_DIR)/user/unionfs/main.c.o \
	$(BUILD_DIR)/user/block/main.c.o \
	$(BUILD_DIR)/user/vfs/main.c.o \
	$(BUILD_DIR)/user/first/main.c.o \
	$(BUILD_DIR)/user/ipcstress/main.c.o \
	$(BUILD_DIR)/user/login/main.c.o \
	$(BUILD_DIR)/user/lxtest/main.S.o \
	$(BUILD_DIR)/user/getdentstest/main.S.o \
	$(BUILD_DIR)/user/execok/main.S.o \
	$(BUILD_DIR)/user/ping/main.c.o
DEPS := $(KERNEL_OBJS:.o=.d) $(USER_OBJS:.o=.d)

.PHONY: all clean run run-kernel run-iso test test-shell test-shell-static test-shell-dynamic test-rootfs-tool iso esp check-tools

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

$(BOOTSTRAP_MODULE): $(BOOTSTRAP_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(BOOTSTRAP_MODULE_OBJS)

$(CONSOLE_MODULE): $(CONSOLE_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(CONSOLE_MODULE_OBJS)

$(NAMES_MODULE): $(NAMES_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(NAMES_MODULE_OBJS)

$(TIME_MODULE): $(TIME_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(TIME_MODULE_OBJS)

$(USER_MODULE): $(USER_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(USER_MODULE_OBJS)

$(LINUX_SERVER_MODULE): $(LINUX_SERVER_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(LINUX_SERVER_MODULE_OBJS)

$(PROC_MODULE): $(PROC_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(PROC_MODULE_OBJS)

$(PROCFS_MODULE): $(PROCFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(PROCFS_MODULE_OBJS)

$(TMPFS_MODULE): $(TMPFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(TMPFS_MODULE_OBJS)

$(DEVFS_MODULE): $(DEVFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(DEVFS_MODULE_OBJS)

$(UTMPFS_MODULE): $(UTMPFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(UTMPFS_MODULE_OBJS)

$(UNIONFS_MODULE): $(UNIONFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(UNIONFS_MODULE_OBJS)

$(BLOCK_MODULE): $(BLOCK_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(BLOCK_MODULE_OBJS)

$(VFS_MODULE): $(VFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(VFS_MODULE_OBJS)

$(FIRST_MODULE): $(FIRST_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(FIRST_MODULE_OBJS)

$(IPCSTRESS_MODULE): $(IPCSTRESS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(IPCSTRESS_MODULE_OBJS)

$(LOGIN_MODULE): $(LOGIN_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(LOGIN_MODULE_OBJS)

$(LXTEST_MODULE): $(LXTEST_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(LXTEST_MODULE_OBJS)

$(GETDENTSTEST_MODULE): $(GETDENTSTEST_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(GETDENTSTEST_MODULE_OBJS)

$(EXECOK_MODULE): $(EXECOK_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(EXECOK_MODULE_OBJS)

$(MUSL_HELLO_MODULE): user/musl-hello/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(DYN_HELLO_MODULE): user/musl-hello/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -O2 -g -Wl,--dynamic-linker=/lib/ld-musl-x86_64.so.1 $< -o $@

$(FPUTEST_MODULE): user/fputest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(PING_MODULE): $(PING_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(PING_MODULE_OBJS)

$(ROOTFS_MODULE): $(ROOTFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(ROOTFS_MODULE_OBJS)

$(ROOTFS_TOOL): tools/mkrootfs.c
	mkdir -p $(dir $@)
	$(CC) -std=c11 -O2 -Wall -Wextra -Werror $< -o $@

$(BLOCK_IMAGE): $(ROOTFS_TOOL) $(ROOTFS_HELLO) $(ROOTFS_SECRET) $(ROOTFS_NESTED) $(ROOTFS_PASSWD) $(ROOTFS_SHADOW) $(ROOTFS_GROUP) $(ROOTFS_INITTAB) $(ROOTFS_EXECS) $(ROOTFS_SPAWNS) $(FIRST_MODULE) $(IPCSTRESS_MODULE) $(LOGIN_MODULE) $(LXTEST_MODULE) $(GETDENTSTEST_MODULE) $(EXECOK_MODULE) $(MUSL_HELLO_MODULE) $(DYN_HELLO_MODULE) $(FPUTEST_MODULE) $(BUSYBOX) $(MUSL_LDSO)
	mkdir -p $(dir $@)
	$(ROOTFS_TOOL) $@ /hello.txt $(ROOTFS_HELLO) /secret.txt $(ROOTFS_SECRET) /usr/share/bunix/nested/hello.txt $(ROOTFS_NESTED) $(ROOTFS_LONG_PATH) $(ROOTFS_NESTED) $(ROOTFS_LONG_EXEC_PATH) $(DYN_HELLO_MODULE) $(ROOTFS_LONG_PROC_EXEC_PATH) $(FIRST_MODULE) /etc/passwd $(ROOTFS_PASSWD) /etc/shadow $(ROOTFS_SHADOW) /etc/group $(ROOTFS_GROUP) /etc/inittab $(ROOTFS_INITTAB) /etc/execs $(ROOTFS_EXECS) /etc/spawns $(ROOTFS_SPAWNS) /lib/ld-musl-x86_64.so.1 $(MUSL_LDSO) /bin/first $(FIRST_MODULE) /bin/ipcstress $(IPCSTRESS_MODULE) /bin/login $(LOGIN_MODULE) /bin/lxtest $(LXTEST_MODULE) /bin/getdentstest $(GETDENTSTEST_MODULE) /bin/execok $(EXECOK_MODULE) /bin/musl-hello $(MUSL_HELLO_MODULE) /bin/dyn-hello $(DYN_HELLO_MODULE) /bin/fputest $(FPUTEST_MODULE) /bin/busybox $(BUSYBOX) --dir /home/kaniini --dir /root --dir /tmp --dir /run --dir /mnt --dir /sys --dir /var/tmp --dir /var/run --symlink $(ROOTFS_LONG_SYMLINK) $(ROOTFS_LONG_PATH) --symlink /lib/libc.musl-x86_64.so.1 /lib/ld-musl-x86_64.so.1 $(ROOTFS_BUSYBOX_LINKS)

$(EFI_BOOT_APP): $(KERNEL) boot/grub-standalone.cfg $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(UTMPFS_MODULE) $(ROOTFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(ESP_DIR)/EFI/BOOT $(ESP_DIR)/boot
	cp $(KERNEL) $(ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=boot/grub-standalone.cfg" \
		"boot/bunixos.kernel=$(KERNEL)" \
		"modules/console.server=$(CONSOLE_MODULE)" \
		"modules/names.server=$(NAMES_MODULE)" \
		"modules/bootstrap.server=$(BOOTSTRAP_MODULE)" \
		"modules/time.server=$(TIME_MODULE)" \
		"modules/user.server=$(USER_MODULE)" \
		"modules/linux.server=$(LINUX_SERVER_MODULE)" \
		"modules/proc.server=$(PROC_MODULE)" \
		"modules/procfs.server=$(PROCFS_MODULE)" \
		"modules/tmpfs.server=$(TMPFS_MODULE)" \
		"modules/devfs.server=$(DEVFS_MODULE)" \
		"modules/utmpfs.server=$(UTMPFS_MODULE)" \
		"modules/rootfs.server=$(ROOTFS_MODULE)" \
		"modules/unionfs.server=$(UNIONFS_MODULE)" \
		"modules/block.server=$(BLOCK_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(EFI_BOOT_IMG): $(KERNEL) boot/grub.cfg $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(UTMPFS_MODULE) $(ROOTFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
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
	cp $(CONSOLE_MODULE) $(ISO_ROOT)/modules/console.server
	cp $(NAMES_MODULE) $(ISO_ROOT)/modules/names.server
	cp $(BOOTSTRAP_MODULE) $(ISO_ROOT)/modules/bootstrap.server
	cp $(TIME_MODULE) $(ISO_ROOT)/modules/time.server
	cp $(USER_MODULE) $(ISO_ROOT)/modules/user.server
	cp $(LINUX_SERVER_MODULE) $(ISO_ROOT)/modules/linux.server
	cp $(PROC_MODULE) $(ISO_ROOT)/modules/proc.server
	cp $(PROCFS_MODULE) $(ISO_ROOT)/modules/procfs.server
	cp $(TMPFS_MODULE) $(ISO_ROOT)/modules/tmpfs.server
	cp $(DEVFS_MODULE) $(ISO_ROOT)/modules/devfs.server
	cp $(UTMPFS_MODULE) $(ISO_ROOT)/modules/utmpfs.server
	cp $(ROOTFS_MODULE) $(ISO_ROOT)/modules/rootfs.server
	cp $(UNIONFS_MODULE) $(ISO_ROOT)/modules/unionfs.server
	cp $(BLOCK_MODULE) $(ISO_ROOT)/modules/block.server
	cp $(VFS_MODULE) $(ISO_ROOT)/modules/vfs.server
	cp $(PING_MODULE) $(ISO_ROOT)/modules/ping.server
	cp $(BLOCK_IMAGE) $(ISO_ROOT)/modules/disk0.img
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
	timeout 60s $(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		-serial file:$(BUILD_DIR)/serial.log -display none -no-reboot; \
		status=$$?; test $$status -eq 0 -o $$status -eq 124
	grep -F "kernel: log level info" $(BUILD_DIR)/serial.log
	grep -F "interrupts: idt loaded" $(BUILD_DIR)/serial.log
	grep -F "timer: pit 100hz" $(BUILD_DIR)/serial.log
	grep -F "interrupts: enabled" $(BUILD_DIR)/serial.log
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
	grep -F "names: init dynamic" $(BUILD_DIR)/serial.log
	grep -F "slab: init caches=" $(BUILD_DIR)/serial.log
	grep -F "buffer: init max=65536" $(BUILD_DIR)/serial.log
	grep -F "ipc: init slab ports messages" $(BUILD_DIR)/serial.log
	grep -F "vm-server: grant_space owner=vm id=1" $(BUILD_DIR)/serial.log
	grep -F "vm-server: memory authority online" $(BUILD_DIR)/serial.log
	grep -F "vm-server: rpc free_frame" $(BUILD_DIR)/serial.log
	grep -F "sched: inherit denied task=" $(BUILD_DIR)/serial.log
	grep -F "handle=2 requested=0x3 rights=0x5" $(BUILD_DIR)/serial.log
	grep -F "kernel: invalid inherited cap handle=2 rights=0x3 for ping" $(BUILD_DIR)/serial.log
	grep -F "names: online" $(BUILD_DIR)/serial.log
	grep -F "names: namespace" $(BUILD_DIR)/serial.log
	grep -F "names: wait" $(BUILD_DIR)/serial.log
	grep -F "names: registered" $(BUILD_DIR)/serial.log
	grep -F "names: resolved" $(BUILD_DIR)/serial.log
	grep -F "time: online" $(BUILD_DIR)/serial.log
	grep -F "time: ready" $(BUILD_DIR)/serial.log
	grep -F "user: online" $(BUILD_DIR)/serial.log
	grep -F "user: ready" $(BUILD_DIR)/serial.log
	grep -F "user: registered" $(BUILD_DIR)/serial.log
	grep -F "user: forked" $(BUILD_DIR)/serial.log
	grep -F "proc: online" $(BUILD_DIR)/serial.log
	grep -F "proc: ready" $(BUILD_DIR)/serial.log
	grep -F "unionfs: online" $(BUILD_DIR)/serial.log
	grep -F "unionfs: mounted" $(BUILD_DIR)/serial.log
	grep -F "proc: exec /sbin/init" $(BUILD_DIR)/serial.log
	grep -F "proc: exec /bin/first" $(BUILD_DIR)/serial.log
	grep -F "proc: exec $(ROOTFS_LONG_PROC_EXEC_PATH)" $(BUILD_DIR)/serial.log
	grep -F "proc: spawned pid=1" $(BUILD_DIR)/serial.log
	grep -F "proc: spawned pid=2" $(BUILD_DIR)/serial.log
	grep -F "first: stdout ready" $(BUILD_DIR)/serial.log
	grep -F "first: argc=1" $(BUILD_DIR)/serial.log
	grep -F "first: argc=4" $(BUILD_DIR)/serial.log
	grep -F "first: argv0=/bin/first" $(BUILD_DIR)/serial.log
	grep -F "first: argv1=alpha" $(BUILD_DIR)/serial.log
	grep -F "first: argv2=beta" $(BUILD_DIR)/serial.log
	grep -F "first: argv3=gamma" $(BUILD_DIR)/serial.log
	grep -F "first: argv65=x065-abcdefghijklmnopqrstuvwxyz0123456789" $(BUILD_DIR)/serial.log
	grep -F "first: argv120=x120-abcdefghijklmnopqrstuvwxyz0123456789" $(BUILD_DIR)/serial.log
	grep -F "first: env PROC_SPAWN_ENV=ok" $(BUILD_DIR)/serial.log
	grep -F "first: aux pagesz=4096" $(BUILD_DIR)/serial.log
	grep -F "first: aux entry=0x400000" $(BUILD_DIR)/serial.log
	grep -F "first: aux phdr=0x" $(BUILD_DIR)/serial.log
	grep -F "first: aux phent=56" $(BUILD_DIR)/serial.log
	grep -F "first: aux phnum=" $(BUILD_DIR)/serial.log
	grep -F "first: aux stdout=2" $(BUILD_DIR)/serial.log
	grep -F "first: aux stderr=2" $(BUILD_DIR)/serial.log
	grep -F "first: aux time=3" $(BUILD_DIR)/serial.log
	grep -F "first: aux proc=4" $(BUILD_DIR)/serial.log
	grep -F "first: large buffer ok" $(BUILD_DIR)/serial.log
	grep -F "linux-server: online" $(BUILD_DIR)/serial.log
	grep -F "linux-server: registered" $(BUILD_DIR)/serial.log
	grep -F "linux-server: openat" $(BUILD_DIR)/serial.log
	grep -F "block: online" $(BUILD_DIR)/serial.log
	grep -F "rootfs: online" $(BUILD_DIR)/serial.log
	grep -F "vfs: online" $(BUILD_DIR)/serial.log
	grep -F "procfs: online" $(BUILD_DIR)/serial.log
	grep -F "tmpfs: online" $(BUILD_DIR)/serial.log
	grep -F "tmpfs: mounted" $(BUILD_DIR)/serial.log
	grep -F "devfs: online" $(BUILD_DIR)/serial.log
	grep -F "devfs: mounted" $(BUILD_DIR)/serial.log
	grep -F "utmpfs: online" $(BUILD_DIR)/serial.log
	grep -F "utmpfs: mounted" $(BUILD_DIR)/serial.log
	grep -F "vfs: ready" $(BUILD_DIR)/serial.log
	grep -F "vfs: mounted translator" $(BUILD_DIR)/serial.log
	grep -F "bootstrap: launching servers" $(BUILD_DIR)/serial.log
	grep -F "bootstrap: names ready" $(BUILD_DIR)/serial.log
	grep -F "bootstrap: fs namespace" $(BUILD_DIR)/serial.log
	grep -F "bootstrap: fs ready" $(BUILD_DIR)/serial.log
	grep -F "bootstrap: linux init exec" $(BUILD_DIR)/serial.log
	grep -F "login: " $(BUILD_DIR)/serial.log
	grep -F "rootfs: module" $(BUILD_DIR)/serial.log
	grep -F "bootstrap: bad cap denied" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server time" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server user" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server linux" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server proc" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server procfs" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server tmpfs" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server devfs" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server utmpfs" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server rootfs" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server block" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server vfs" $(BUILD_DIR)/serial.log
	grep -F "kernel: launching module server ping" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server ping" $(BUILD_DIR)/serial.log
	grep -F "kernel: starting module server ping image=0x" $(BUILD_DIR)/serial.log
	grep -F "ping: online" $(BUILD_DIR)/serial.log

test-shell: $(EFI_BOOT_APP)
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		sh tools/test-shell.sh

test-shell-dynamic:
	$(MAKE) -B test-shell BUSYBOX=$(BUSYBOX_DYNAMIC)

test-shell-static:
	$(MAKE) -B test-shell BUSYBOX=$(BUSYBOX_STATIC)

test-rootfs-tool: $(ROOTFS_TOOL)
	sh tools/test-rootfs-tool.sh $(ROOTFS_TOOL)

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
