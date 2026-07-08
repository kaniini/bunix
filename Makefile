ARCH ?= x86_64
BUILD_DIR := build
ARCH_BUILD_DIR := $(BUILD_DIR)/$(ARCH)
ISO_ROOT := $(BUILD_DIR)/iso
ESP_DIR := $(BUILD_DIR)/esp
ALPINE_ESP_DIR := $(BUILD_DIR)/esp-alpine
ALPINE_EFI_BOOT_APP := $(ALPINE_ESP_DIR)/EFI/BOOT/BOOTX64.EFI
VIRTIO_BLK_TEST_ESP_DIR := $(BUILD_DIR)/esp-virtio-blk
VIRTIO_BLK_TEST_EFI_BOOT_APP := $(VIRTIO_BLK_TEST_ESP_DIR)/EFI/BOOT/BOOTX64.EFI
VIRTIO_BLK_TEST_GRUB_STANDALONE_CFG := $(BUILD_DIR)/grub-standalone-virtio-blk.cfg
VIRTIO_BLK_TEST_CMDLINE ?= log=info virtio-blk-test
VIRTIO_NET_TEST_ESP_DIR := $(BUILD_DIR)/esp-virtio-net
VIRTIO_NET_TEST_EFI_BOOT_APP := $(VIRTIO_NET_TEST_ESP_DIR)/EFI/BOOT/BOOTX64.EFI
VIRTIO_NET_TEST_GRUB_STANDALONE_CFG := $(BUILD_DIR)/grub-standalone-virtio-net.cfg
VIRTIO_NET_TEST_CMDLINE ?= log=info virtio-net-test
EXT2_TEST_ESP_DIR := $(BUILD_DIR)/esp-ext2
EXT2_TEST_EFI_BOOT_APP := $(EXT2_TEST_ESP_DIR)/EFI/BOOT/BOOTX64.EFI
EXT2_TEST_GRUB_STANDALONE_CFG := $(BUILD_DIR)/grub-standalone-ext2.cfg
EXT2_TEST_CMDLINE ?= log=info ext2-test
EXT2_FSCK_TEST_ESP_DIR := $(BUILD_DIR)/esp-ext2-fsck
EXT2_FSCK_TEST_EFI_BOOT_APP := $(EXT2_FSCK_TEST_ESP_DIR)/EFI/BOOT/BOOTX64.EFI
EXT2_FSCK_TEST_GRUB_STANDALONE_CFG := $(BUILD_DIR)/grub-standalone-ext2-fsck.cfg
EXT2_FSCK_TEST_CMDLINE ?= log=info virtio-blk-test virtio-blk-block-test ext2-fsck-test
EFI_BOOT_IMG := $(BUILD_DIR)/bunixos-efi.iso
EFI_BOOT_APP := $(ESP_DIR)/EFI/BOOT/BOOTX64.EFI
GRUB_CFG := $(BUILD_DIR)/grub.cfg
GRUB_STANDALONE_CFG := $(BUILD_DIR)/grub-standalone.cfg
KERNEL := $(ARCH_BUILD_DIR)/bunixos.kernel
RISCV64_KERNEL := $(BUILD_DIR)/riscv64/bunixos.kernel
RISCV64_SERIAL_LOG := $(BUILD_DIR)/riscv64-serial.log
RISCV64_QEMU ?= qemu-system-riscv64
RISCV64_CC ?= clang
RISCV64_CC_TARGET_FLAGS ?= --target=riscv64-alpine-linux-musl
RISCV64_LD ?= riscv64-alpine-linux-musl-ld
RISCV64_OBJDUMP ?= llvm-objdump
RISCV64_READELF ?= llvm-readelf
RISCV64_USER_ABI_MODULE := $(BUILD_DIR)/riscv64/modules/abi-smoke.user
RISCV64_BOOTPKG := $(BUILD_DIR)/riscv64/bootpkg.img
RISCV64_BOOTPKG_MULTI := $(BUILD_DIR)/riscv64/bootpkg-multi.img
RISCV64_KERNEL_CMDLINE ?= log=info riscv64-bootpkg-test
RISCV64_MUSLCC_PREFIX ?= $(BUILD_DIR)/toolchains/riscv64-linux-musl-cross
RISCV64_MUSLCC_GCC := $(RISCV64_MUSLCC_PREFIX)/bin/riscv64-linux-musl-gcc
RISCV64_MUSLCC_SYSROOT := $(RISCV64_MUSLCC_PREFIX)/riscv64-linux-musl
RISCV64_MUSLCC_GCC_LIBDIR := $(RISCV64_MUSLCC_PREFIX)/lib/gcc/riscv64-linux-musl/11.2.1
RISCV64_MUSL_HELLO_MODULE := $(BUILD_DIR)/riscv64/modules/musl-hello.user
RISCV64_LINUX_SERVER_MODULE := $(BUILD_DIR)/riscv64/modules/linux.server
RISCV64_BOOTSTRAP_MODULE := $(BUILD_DIR)/riscv64/modules/bootstrap.server
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
SYSFS_MODULE := $(BUILD_DIR)/modules/sysfs.server
SYSFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/sysfs/main.c.o
UTMPFS_MODULE := $(BUILD_DIR)/modules/utmpfs.server
UTMPFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/utmpfs/main.c.o
UNIONFS_MODULE := $(BUILD_DIR)/modules/unionfs.server
UNIONFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/unionfs/main.c.o
EXT2_MODULE := $(BUILD_DIR)/modules/ext2.server
EXT2_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/ext2/main.c.o
SQUASHFS_MODULE := $(BUILD_DIR)/modules/squashfs.server
SQUASHFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/squashfs/main.c.o
BLOCK_MODULE := $(BUILD_DIR)/modules/block.server
BLOCK_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/block/main.c.o
PCI_MODULE := $(BUILD_DIR)/modules/pci.server
PCI_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/pci/main.c.o
USB_BUS_MODULE := $(BUILD_DIR)/modules/usb-bus.server
USB_BUS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/usb-bus/main.c.o
USB_SYNTH_MODULE := $(BUILD_DIR)/modules/usb-synth.server
USB_SYNTH_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/usb-synth/main.c.o
XHCI_MODULE := $(BUILD_DIR)/modules/xhci.server
XHCI_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/xhci/main.c.o
VIRTIO_BUS_MODULE := $(BUILD_DIR)/modules/virtio-bus.server
VIRTIO_BUS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/virtio-bus/main.c.o
NET_MODULE := $(BUILD_DIR)/modules/net.server
NET_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/net/main.c.o
NETCFG_MODULE := $(BUILD_DIR)/modules/netcfg.server
NETCFG_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/netcfg/main.c.o
VIRTIO_BLK_MODULE := $(BUILD_DIR)/modules/virtio-blk.server
VIRTIO_BLK_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/virtio-blk/main.c.o
VIRTIO_NET_MODULE := $(BUILD_DIR)/modules/virtio-net.server
VIRTIO_NET_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/virtio-net/main.c.o
VFS_MODULE := $(BUILD_DIR)/modules/vfs.server
VFS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/vfs/main.c.o
FIRST_MODULE := $(BUILD_DIR)/modules/first.user
FIRST_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/first/main.c.o
ALLOCTEST_MODULE := $(BUILD_DIR)/modules/alloctest.user
ALLOCTEST_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/alloctest/main.c.o
IPCSTRESS_MODULE := $(BUILD_DIR)/modules/ipcstress.user
IPCSTRESS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/ipcstress/main.c.o
LOGIN_MODULE := $(BUILD_DIR)/modules/login.user
LOGIN_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/login/main.c.o
LXTEST_MODULE := $(BUILD_DIR)/modules/lxtest.user
LXTEST_MODULE_OBJS := $(BUILD_DIR)/user/lxtest/main.S.o
GETDENTSTEST_MODULE := $(BUILD_DIR)/modules/getdentstest.user
GETDENTSTEST_MODULE_OBJS := $(BUILD_DIR)/user/getdentstest/main.S.o
VFORKSTRESS_MODULE := $(BUILD_DIR)/modules/vforkstress.user
VFORKSTRESS_MODULE_OBJS := $(BUILD_DIR)/user/vforkstress/main.S.o
EXECOK_MODULE := $(BUILD_DIR)/modules/execok.user
EXECOK_MODULE_OBJS := $(BUILD_DIR)/user/execok/main.S.o
READBIG_MODULE := $(BUILD_DIR)/modules/readbig.user
READBIG_MODULE_OBJS := $(BUILD_DIR)/user/readbig/main.S.o
MMAPBIG_MODULE := $(BUILD_DIR)/modules/mmapbig.user
MMAPBIG_MODULE_OBJS := $(BUILD_DIR)/user/mmapbig/main.S.o
MMAPHUGE_MODULE := $(BUILD_DIR)/modules/mmaphuge.user
MMAPHUGE_MODULE_OBJS := $(BUILD_DIR)/user/mmaphuge/main.S.o
EXECBIG_MODULE := $(BUILD_DIR)/modules/execbig.user
EXECBIG_SIZE := 2097153
PHDRSTRESS_MODULE := $(BUILD_DIR)/modules/phdrstress.user
PHDRSTRESS_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/phdrstress/main.c.o
MUSL_HELLO_MODULE := $(BUILD_DIR)/modules/musl-hello.user
FPUTEST_MODULE := $(BUILD_DIR)/modules/fputest.user
IOVTEST_MODULE := $(BUILD_DIR)/modules/iovtest.user
FCHMODATTEST_MODULE := $(BUILD_DIR)/modules/fchmodattest.user
WAITPGIDTEST_MODULE := $(BUILD_DIR)/modules/waitpgidtest.user
EXECLONGTEST_MODULE := $(BUILD_DIR)/modules/execlongtest.user
AUXIDTEST_MODULE := $(BUILD_DIR)/modules/auxidtest.user
PATHMAXTEST_MODULE := $(BUILD_DIR)/modules/pathmaxtest.user
PATHERRTEST_MODULE := $(BUILD_DIR)/modules/patherrtest.user
STATIDTEST_MODULE := $(BUILD_DIR)/modules/statidtest.user
FCNTLLOCKTEST_MODULE := $(BUILD_DIR)/modules/fcntllocktest.user
SIGNALTEST_MODULE := $(BUILD_DIR)/modules/signaltest.user
FAULTTEST_MODULE := $(BUILD_DIR)/modules/faulttest.user
FAULTTEST_MODULE_OBJ := $(BUILD_DIR)/user/faulttest/main.S.o
SYSRACETEST_MODULE := $(BUILD_DIR)/modules/sysracetest.user
SCHEDSTRESS_MODULE := $(BUILD_DIR)/modules/schedstress.user
UPTIMETEST_MODULE := $(BUILD_DIR)/modules/uptimetest.user
NETTEST_MODULE := $(BUILD_DIR)/modules/nettest.user
NETDHCP_MODULE := $(BUILD_DIR)/modules/bunix-udhcpc-script.user
DYN_HELLO_MODULE := $(BUILD_DIR)/modules/dyn-hello.user
BUSYBOX_DYNAMIC ?= /bin/busybox
BUSYBOX_STATIC ?= /usr/bin/busybox.static
BUSYBOX ?= $(BUSYBOX_DYNAMIC)
MUSL_LDSO ?= /lib/ld-musl-x86_64.so.1
PING_MODULE := $(BUILD_DIR)/modules/ping.server
PING_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/ping/main.c.o
SYNTHETIC_SQUASHFS_IMAGE := $(BUILD_DIR)/modules/disk0.sqfs
ALPINE_SQUASHFS_IMAGE := $(BUILD_DIR)/modules/alpine-disk0.sqfs
ROOTFS_FLAVOR ?= squashfs
BLOCK_IMAGE := $(if $(filter alpine-squashfs,$(ROOTFS_FLAVOR)),$(ALPINE_SQUASHFS_IMAGE),$(SYNTHETIC_SQUASHFS_IMAGE))
VIRTIO_BLOCK_IMAGE ?= $(BLOCK_IMAGE)
VIRTIO_BLK_TEST_IMAGE := $(BUILD_DIR)/virtio-blk-test.img
EXT2_TEST_IMAGE := $(BUILD_DIR)/modules/ext2-test.img
EXT2_FSCK_TEST_IMAGE := $(BUILD_DIR)/ext2-fsck-test.img
QEMU_VIRTIO_BLK_ARGS := -drive if=none,id=bunix-virtio0,format=raw,readonly=on,file=$(VIRTIO_BLOCK_IMAGE) -device virtio-blk-pci,disable-legacy=on,drive=bunix-virtio0,bus=pcie.0,addr=0x6
QEMU_VIRTIO_BLK_TEST_ARGS := -drive if=none,id=bunix-virtio0,format=raw,file=$(VIRTIO_BLK_TEST_IMAGE) -device virtio-blk-pci,disable-legacy=on,drive=bunix-virtio0,bus=pcie.0,addr=0x6
QEMU_EXT2_FSCK_TEST_ARGS := -drive if=none,id=bunix-virtio0,format=raw,file=$(EXT2_FSCK_TEST_IMAGE) -device virtio-blk-pci,disable-legacy=on,drive=bunix-virtio0,bus=pcie.0,addr=0x6
QEMU_VIRTIO_NET_ARGS := -netdev user,id=bunix-net0,restrict=on -device virtio-net-pci,disable-legacy=on,netdev=bunix-net0,mac=52:54:00:18:00:01,bus=pcie.0,addr=0x7
QEMU_VIRTIO_NET_EXTERNAL_ARGS := -netdev user,id=bunix-net0 -device virtio-net-pci,disable-legacy=on,netdev=bunix-net0,mac=52:54:00:18:00:01,bus=pcie.0,addr=0x7
QEMU_VIRTIO_NET_SOCKET_MCAST ?= 230.18.0.1:18100
QEMU_VIRTIO_NET_SOCKET_ARGS := -netdev socket,id=bunix-net0,mcast=$(QEMU_VIRTIO_NET_SOCKET_MCAST) -device virtio-net-pci,disable-legacy=on,netdev=bunix-net0,mac=52:54:00:18:00:01,bus=pcie.0,addr=0x7
QEMU_XHCI_ARGS := -device qemu-xhci,id=bunix-xhci,bus=pcie.0,addr=0x8 -device usb-kbd,bus=bunix-xhci.0
QEMU_TIMEOUT ?= $(if $(filter alpine-squashfs,$(ROOTFS_FLAVOR)),120s,60s)
TEST_BOOT_MARKERS := $(if $(filter alpine-squashfs,$(ROOTFS_FLAVOR)),tools/test-boot-markers-alpine-squashfs.txt,tools/test-boot-markers-squashfs.txt)
ROOTFS_FLAVOR_STAMP := $(BUILD_DIR)/rootfs-flavor.stamp
PARALLEL_TEST_SET := $(if $(BUNIX_TEST_SET),$(BUNIX_TEST_SET),all)
PARALLEL_ALPINE_ESP := $(if $(filter all openrc,$(PARALLEL_TEST_SET)),$(ALPINE_EFI_BOOT_APP))
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
ROOTFS_SHEBANGTEST := modules/shebangtest.sh
ROOTFS_SHEBANGLOOP_A := modules/shebangloop-a.sh
ROOTFS_SHEBANGLOOP_B := modules/shebangloop-b.sh
ROOTFS_SHEBANGBAD := modules/shebangbad.sh
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
	--symlink /bin/umount /bin/busybox \
	--symlink /bin/free /bin/busybox \
	--symlink /bin/df /bin/busybox \
	--symlink /bin/ps /bin/busybox \
	--symlink /bin/top /bin/busybox \
	--symlink /bin/id /bin/busybox \
	--symlink /bin/kill /bin/busybox \
	--symlink /bin/echo /bin/busybox \
	--symlink /bin/env /bin/busybox \
	--symlink /bin/stty /bin/busybox \
	--symlink /bin/pwd /bin/busybox \
	--symlink /bin/poweroff /bin/busybox \
	--symlink /sbin/poweroff /bin/busybox \
	--symlink /sbin/halt /bin/busybox \
	--symlink /sbin/reboot /bin/busybox

ifeq ($(ARCH),x86_64)
CC ?= gcc
MUSL_CC ?= x86_64-alpine-linux-musl-gcc
LD ?= ld
OBJDUMP ?= objdump
READELF ?= readelf
QEMU ?= qemu-system-x86_64
KERNEL_LINKER := linker.ld
KERNEL_LD_EMULATION := elf_x86_64
KERNEL_OBJDUMP_CHECK := multiboot
KERNEL_ARCH_SRCS := \
	arch/$(ARCH)/boot/multiboot2.S \
	arch/$(ARCH)/ap_trampoline.S \
	arch/$(ARCH)/interrupts.c \
	arch/$(ARCH)/power.c \
	arch/$(ARCH)/smp.c \
	arch/$(ARCH)/interrupts.S \
	arch/$(ARCH)/thread.c \
	arch/$(ARCH)/thread.S \
	arch/$(ARCH)/syscall.S \
	arch/$(ARCH)/user.c \
	arch/$(ARCH)/vm.c
ARCH_CFLAGS := -m64 -mno-red-zone -mno-sse -mno-sse2
ARCH_ASFLAGS := -m64
else ifeq ($(ARCH),riscv64)
CC := $(RISCV64_CC)
MUSL_CC ?= riscv64-alpine-linux-musl-gcc
LD := $(RISCV64_LD)
OBJDUMP := $(RISCV64_OBJDUMP)
READELF := $(RISCV64_READELF)
QEMU ?= qemu-system-riscv64
KERNEL_LINKER := arch/riscv64/linker.ld
KERNEL_LD_EMULATION := elf64lriscv
KERNEL_OBJDUMP_CHECK :=
KERNEL_ARCH_SRCS := \
	arch/riscv64/boot/start.S \
	arch/riscv64/console.c \
	arch/riscv64/early.c \
	arch/riscv64/fdt.c \
	arch/riscv64/interrupts.c \
	arch/riscv64/power.c \
	arch/riscv64/smp.c \
	arch/riscv64/trap.S \
	arch/riscv64/thread.c \
	arch/riscv64/thread.S \
	arch/riscv64/user.c \
	arch/riscv64/vm.c
ARCH_CFLAGS := $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany
ARCH_ASFLAGS := $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany
else
$(error unsupported ARCH=$(ARCH))
endif
SMP ?= 2
GRUB_MKRESCUE ?= grub-mkrescue
GRUB_MKSTANDALONE ?= grub-mkstandalone
OVMF_CODE ?= /usr/share/OVMF/OVMF_CODE.fd
KERNEL_CMDLINE ?= log=info
EFFECTIVE_KERNEL_CMDLINE = $(KERNEL_CMDLINE)$(if $(filter squashfs alpine-squashfs,$(ROOTFS_FLAVOR)), squashfs-root)

CFLAGS := $(ARCH_CFLAGS) -std=c11 -O2 -g -ffreestanding -fno-stack-protector \
	-fno-pic -fno-pie -fno-builtin \
	-Iarch/$(ARCH)/include -Ikernel \
	-Wall -Wextra -Werror -MMD -MP
ASFLAGS := $(ARCH_ASFLAGS) -g -ffreestanding -Iarch/$(ARCH)/include -Ikernel -MMD -MP
LDFLAGS := -m $(KERNEL_LD_EMULATION) -nostdlib -T $(KERNEL_LINKER)
USER_CFLAGS := -m64 -std=c11 -O2 -g -ffreestanding -fno-stack-protector \
	-fno-pic -fno-pie -fno-builtin -mno-red-zone \
	-mno-sse -mno-sse2 \
	-Iuser/include -Wall -Wextra -Werror -MMD -MP
USER_ASFLAGS := -m64 -g -ffreestanding -fno-pic -fno-pie \
	-Iuser/include -MMD -MP

KERNEL_GENERIC_SRCS_x86_64 := \
	kernel/main.c \
	kernel/buffer.c \
	kernel/cmdline.c \
	kernel/console.c \
	kernel/elf.c \
	kernel/ipc.c \
	kernel/multiboot2.c \
	kernel/name.c \
	kernel/pmm.c \
	kernel/pmm_multiboot2.c \
	kernel/sched.c \
	kernel/slab.c \
	kernel/server.c \
	kernel/server_multiboot2.c \
	kernel/spinlock.c \
	kernel/string.c \
	kernel/timer.c \
	kernel/vm.c \
	servers/vm/vm.c
KERNEL_GENERIC_SRCS_riscv64 := \
	kernel/buffer.c \
	kernel/cmdline.c \
	kernel/elf.c \
	kernel/ipc.c \
	kernel/name.c \
	kernel/pmm.c \
	kernel/sched.c \
	kernel/slab.c \
	kernel/server.c \
	kernel/spinlock.c \
	kernel/string.c \
	kernel/timer.c \
	kernel/vm.c \
	servers/vm/vm.c
KERNEL_GENERIC_SRCS := $(KERNEL_GENERIC_SRCS_$(ARCH))
KERNEL_SRCS := \
	$(KERNEL_ARCH_SRCS) \
	$(KERNEL_GENERIC_SRCS)

KERNEL_OBJS := $(KERNEL_SRCS:%=$(BUILD_DIR)/$(ARCH)/%.o)
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
	$(BUILD_DIR)/user/sysfs/main.c.o \
	$(BUILD_DIR)/user/utmpfs/main.c.o \
	$(BUILD_DIR)/user/unionfs/main.c.o \
	$(BUILD_DIR)/user/ext2/main.c.o \
	$(BUILD_DIR)/user/squashfs/main.c.o \
	$(BUILD_DIR)/user/block/main.c.o \
	$(BUILD_DIR)/user/pci/main.c.o \
	$(BUILD_DIR)/user/usb-bus/main.c.o \
	$(BUILD_DIR)/user/usb-synth/main.c.o \
	$(BUILD_DIR)/user/xhci/main.c.o \
	$(BUILD_DIR)/user/virtio-bus/main.c.o \
	$(BUILD_DIR)/user/net/main.c.o \
	$(BUILD_DIR)/user/netcfg/main.c.o \
	$(BUILD_DIR)/user/virtio-blk/main.c.o \
	$(BUILD_DIR)/user/virtio-net/main.c.o \
	$(BUILD_DIR)/user/vfs/main.c.o \
	$(BUILD_DIR)/user/first/main.c.o \
	$(BUILD_DIR)/user/alloctest/main.c.o \
	$(BUILD_DIR)/user/ipcstress/main.c.o \
	$(BUILD_DIR)/user/login/main.c.o \
	$(BUILD_DIR)/user/lxtest/main.S.o \
	$(BUILD_DIR)/user/getdentstest/main.S.o \
	$(BUILD_DIR)/user/vforkstress/main.S.o \
	$(BUILD_DIR)/user/execok/main.S.o \
	$(BUILD_DIR)/user/readbig/main.S.o \
	$(BUILD_DIR)/user/mmapbig/main.S.o \
	$(BUILD_DIR)/user/phdrstress/main.c.o \
	$(BUILD_DIR)/user/ping/main.c.o
DEPS := $(KERNEL_OBJS:.o=.d) $(USER_OBJS:.o=.d)

.PHONY: all clean run run-alpine-net run-virtio run-virtio-net run-kernel run-iso run-riscv64-early riscv64-muslcc-toolchain test test-alpine-rootfs test-boot test-boot-ext2 test-boot-ext2-fsck test-boot-ext2-root test-boot-riscv64-early test-riscv64-bootpkg test-riscv64-user-abi test-boot-usb test-boot-usb-synth test-boot-xhci-discovery test-boot-virtio test-boot-virtio-net test-boot-virtio-net-dhcp test-boot-virtio-net-ifup test-boot-virtio-net-ifup-run test-boot-virtio-net-networking test-boot-virtio-net-networking-run test-boot-virtio-net-socket-peer test-boot-virtio-net-external-ping test-boot-virtio-net-external-ping-run test-boot-virtio-blk test-boot-virtio-blk-irq test-boot-virtio-blk-backend test-boot-virtio-blk-irq-backend test-command test-shell test-shell-part test-shell-squashfs-rootfs test-smoke test-smoke-parallel test-shell-parallel test-parallel test-prune-artifacts test-shell-static test-shell-dynamic list-shell-shards audit-linux-syscalls security-audit-check iso esp check-tools FORCE

all: $(KERNEL)

$(KERNEL): $(KERNEL_OBJS) $(KERNEL_LINKER)
	mkdir -p $(dir $@)
	$(LD) $(LDFLAGS) -o $@ $(KERNEL_OBJS)
ifneq ($(KERNEL_OBJDUMP_CHECK),)
	$(OBJDUMP) -h $@ | awk '/multiboot/ { if (strtonum("0x" $$6) >= 0x8000) exit 1 }'
endif

$(BUILD_DIR)/$(ARCH)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/$(ARCH)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

$(BUILD_DIR)/%.c.o: %.c
	mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

$(BUILD_DIR)/%.S.o: %.S
	mkdir -p $(dir $@)
	$(CC) $(ASFLAGS) -c $< -o $@

iso: $(EFI_BOOT_IMG)

esp: $(EFI_BOOT_APP)

ifneq ($(ESP_DIR),$(ALPINE_ESP_DIR))
$(ALPINE_EFI_BOOT_APP): FORCE
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs ESP_DIR=$(ALPINE_ESP_DIR) esp
endif

$(BUILD_DIR)/user/%.c.o: user/%.c
	mkdir -p $(dir $@)
	$(CC) $(USER_CFLAGS) -c $< -o $@

$(BUILD_DIR)/user/%.S.o: user/%.S
	mkdir -p $(dir $@)
	$(CC) $(USER_ASFLAGS) -c $< -o $@

$(RISCV64_USER_ABI_MODULE): user/crt0-riscv64.S user/riscv64-abi/main.c user/user.ld user/include/bunix/syscall.h Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-g -ffreestanding -fno-pic -fno-pie -Iuser/include -MMD -MP \
		-c user/crt0-riscv64.S -o $(BUILD_DIR)/riscv64/user/crt0-riscv64.S.o
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include \
		-Wall -Wextra -Werror -MMD -MP \
		-c user/riscv64-abi/main.c -o $(BUILD_DIR)/riscv64/user/riscv64-abi.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(BUILD_DIR)/riscv64/user/crt0-riscv64.S.o \
		$(BUILD_DIR)/riscv64/user/riscv64-abi.c.o

test-riscv64-user-abi: $(RISCV64_USER_ABI_MODULE)
	$(RISCV64_READELF) -h $(RISCV64_USER_ABI_MODULE) | grep -F "RISC-V" >/dev/null

riscv64-muslcc-toolchain:
	sh tools/setup-riscv64-muslcc.sh $(RISCV64_MUSLCC_PREFIX)

$(RISCV64_MUSLCC_GCC): tools/setup-riscv64-muslcc.sh
	sh tools/setup-riscv64-muslcc.sh $(RISCV64_MUSLCC_PREFIX)

$(RISCV64_MUSL_HELLO_MODULE): user/musl-hello/main.c $(RISCV64_MUSLCC_GCC) Makefile
	mkdir -p $(dir $@)
	$(RISCV64_CC) --target=riscv64-linux-musl -march=rv64gc -mabi=lp64d \
		-isystem $(RISCV64_MUSLCC_SYSROOT)/include \
		-static -fuse-ld=/usr/bin/$(RISCV64_LD) \
		-B $(RISCV64_MUSLCC_SYSROOT)/lib \
		-B $(RISCV64_MUSLCC_GCC_LIBDIR) \
		-L $(RISCV64_MUSLCC_SYSROOT)/lib \
		-L $(RISCV64_MUSLCC_GCC_LIBDIR) \
		-O2 -g $< -o $@
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_LINUX_SERVER_MODULE): user/crt0-riscv64.S user/riscv64-linux/main.c user/user.ld user/include/bunix/syscall.h Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include \
		-c user/crt0-riscv64.S -o $(BUILD_DIR)/riscv64/user/crt0-riscv64.S.o
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/riscv64-linux/main.c -o $(BUILD_DIR)/riscv64/user/riscv64-linux.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(BUILD_DIR)/riscv64/user/crt0-riscv64.S.o \
		$(BUILD_DIR)/riscv64/user/riscv64-linux.c.o

$(RISCV64_BOOTSTRAP_MODULE): user/crt0-riscv64.S user/riscv64-bootstrap/main.c user/user.ld user/include/bunix/syscall.h Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include \
		-c user/crt0-riscv64.S -o $(BUILD_DIR)/riscv64/user/crt0-riscv64.S.o
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/riscv64-bootstrap/main.c -o $(BUILD_DIR)/riscv64/user/riscv64-bootstrap.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(BUILD_DIR)/riscv64/user/crt0-riscv64.S.o \
		$(BUILD_DIR)/riscv64/user/riscv64-bootstrap.c.o

$(RISCV64_BOOTPKG): $(RISCV64_BOOTSTRAP_MODULE) $(RISCV64_USER_ABI_MODULE) $(RISCV64_LINUX_SERVER_MODULE) $(RISCV64_MUSL_HELLO_MODULE) tools/build-riscv64-bootpkg.sh
	sh tools/build-riscv64-bootpkg.sh $@ --cmdline "$(RISCV64_KERNEL_CMDLINE)" \
		$(RISCV64_BOOTSTRAP_MODULE) bootstrap \
		$(RISCV64_USER_ABI_MODULE) abi-smoke.user \
		$(RISCV64_LINUX_SERVER_MODULE) linux \
		$(RISCV64_MUSL_HELLO_MODULE) /bin/musl-hello

$(RISCV64_BOOTPKG_MULTI): $(RISCV64_BOOTSTRAP_MODULE) $(RISCV64_USER_ABI_MODULE) $(RISCV64_LINUX_SERVER_MODULE) $(RISCV64_MUSL_HELLO_MODULE) tools/build-riscv64-bootpkg.sh
	sh tools/build-riscv64-bootpkg.sh $@ --cmdline "$(RISCV64_KERNEL_CMDLINE)" \
		$(RISCV64_USER_ABI_MODULE) disk0 \
		$(RISCV64_BOOTSTRAP_MODULE) bootstrap \
		$(RISCV64_USER_ABI_MODULE) abi-smoke.user \
		$(RISCV64_LINUX_SERVER_MODULE) linux \
		$(RISCV64_MUSL_HELLO_MODULE) /bin/musl-hello

test-riscv64-bootpkg: $(RISCV64_BOOTPKG) $(RISCV64_BOOTPKG_MULTI)
	grep -aF "BUNIX-RV64-BOOTPKG" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "cmdline $(RISCV64_KERNEL_CMDLINE)" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module bootstrap" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module abi-smoke.user" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module linux" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module /bin/musl-hello" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "cmdline $(RISCV64_KERNEL_CMDLINE)" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module disk0" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module bootstrap" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module abi-smoke.user" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module linux" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module /bin/musl-hello" $(RISCV64_BOOTPKG_MULTI) >/dev/null

$(BOOTSTRAP_MODULE): $(BOOTSTRAP_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(BOOTSTRAP_MODULE_OBJS)

$(ALLOCTEST_MODULE): $(ALLOCTEST_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(ALLOCTEST_MODULE_OBJS)

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

$(SYSFS_MODULE): $(SYSFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(SYSFS_MODULE_OBJS)

$(UTMPFS_MODULE): $(UTMPFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(UTMPFS_MODULE_OBJS)

$(UNIONFS_MODULE): $(UNIONFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(UNIONFS_MODULE_OBJS)

$(EXT2_MODULE): $(EXT2_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(EXT2_MODULE_OBJS)

$(SQUASHFS_MODULE): $(SQUASHFS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(SQUASHFS_MODULE_OBJS)

$(BLOCK_MODULE): $(BLOCK_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(BLOCK_MODULE_OBJS)

$(PCI_MODULE): $(PCI_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(PCI_MODULE_OBJS)

$(USB_BUS_MODULE): $(USB_BUS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(USB_BUS_MODULE_OBJS)

$(USB_SYNTH_MODULE): $(USB_SYNTH_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(USB_SYNTH_MODULE_OBJS)

$(XHCI_MODULE): $(XHCI_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(XHCI_MODULE_OBJS)

$(VIRTIO_BUS_MODULE): $(VIRTIO_BUS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(VIRTIO_BUS_MODULE_OBJS)

$(NET_MODULE): $(NET_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(NET_MODULE_OBJS)

$(VIRTIO_BLK_MODULE): $(VIRTIO_BLK_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(VIRTIO_BLK_MODULE_OBJS)

$(VIRTIO_NET_MODULE): $(VIRTIO_NET_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(VIRTIO_NET_MODULE_OBJS)

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

$(NETDHCP_MODULE): $(USER_CRT0_OBJ) $(BUILD_DIR)/user/netdhcp/main.c.o user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(USER_CRT0_OBJ) $(BUILD_DIR)/user/netdhcp/main.c.o

$(LXTEST_MODULE): $(LXTEST_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(LXTEST_MODULE_OBJS)

$(GETDENTSTEST_MODULE): $(GETDENTSTEST_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(GETDENTSTEST_MODULE_OBJS)

$(VFORKSTRESS_MODULE): $(VFORKSTRESS_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(VFORKSTRESS_MODULE_OBJS)

$(EXECOK_MODULE): $(EXECOK_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(EXECOK_MODULE_OBJS)

$(READBIG_MODULE): $(READBIG_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(READBIG_MODULE_OBJS)

$(MMAPBIG_MODULE): $(MMAPBIG_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(MMAPBIG_MODULE_OBJS)

$(MMAPHUGE_MODULE): $(MMAPHUGE_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(MMAPHUGE_MODULE_OBJS)

$(PHDRSTRESS_MODULE): $(PHDRSTRESS_MODULE_OBJS) user/phdrstress/phdrstress.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/phdrstress/phdrstress.ld -o $@ $(PHDRSTRESS_MODULE_OBJS)
	$(READELF) -h $@ | awk '/Number of program headers:/ { if ($$5 <= 16) exit 1; found = 1 } END { if (!found) exit 1 }'

$(MUSL_HELLO_MODULE): user/musl-hello/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(DYN_HELLO_MODULE): user/musl-hello/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -O2 -g -Wl,--dynamic-linker=/lib/ld-musl-x86_64.so.1 $< -o $@

$(EXECBIG_MODULE): $(EXECOK_MODULE) Makefile
	mkdir -p $(dir $@)
	chmod u+w $@ 2>/dev/null || true
	cp $(EXECOK_MODULE) $@
	truncate -s $(EXECBIG_SIZE) $@
	chmod 0555 $@
	wc -c < $@ | awk '{ if ($$1 <= 2097152) exit 1 }'

$(FPUTEST_MODULE): user/fputest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(IOVTEST_MODULE): user/iovtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(FCHMODATTEST_MODULE): user/fchmodattest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(WAITPGIDTEST_MODULE): user/waitpgidtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(EXECLONGTEST_MODULE): user/execlongtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(AUXIDTEST_MODULE): user/auxidtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(PATHMAXTEST_MODULE): user/pathmaxtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(PATHERRTEST_MODULE): user/patherrtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(STATIDTEST_MODULE): user/statidtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(FCNTLLOCKTEST_MODULE): user/fcntllocktest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(SIGNALTEST_MODULE): user/signaltest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(FAULTTEST_MODULE): $(FAULTTEST_MODULE_OBJ) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(FAULTTEST_MODULE_OBJ)

$(SYSRACETEST_MODULE): user/sysracetest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(SCHEDSTRESS_MODULE): user/schedstress/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(UPTIMETEST_MODULE): user/uptimetest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(NETTEST_MODULE): user/nettest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(PING_MODULE): $(PING_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(PING_MODULE_OBJS)

$(NETCFG_MODULE): $(NETCFG_MODULE_OBJS) user/user.ld Makefile
	mkdir -p $(dir $@)
	$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $@ $(NETCFG_MODULE_OBJS)

$(SYNTHETIC_SQUASHFS_IMAGE): tools/build-synthetic-squashfs-rootfs.sh $(ROOTFS_HELLO) $(ROOTFS_SECRET) $(ROOTFS_NESTED) $(ROOTFS_PASSWD) $(ROOTFS_SHADOW) $(ROOTFS_GROUP) $(ROOTFS_INITTAB) $(ROOTFS_EXECS) $(ROOTFS_SPAWNS) $(ROOTFS_SHEBANGTEST) $(ROOTFS_SHEBANGLOOP_A) $(ROOTFS_SHEBANGLOOP_B) $(ROOTFS_SHEBANGBAD) $(FIRST_MODULE) $(ALLOCTEST_MODULE) $(IPCSTRESS_MODULE) $(LOGIN_MODULE) $(LXTEST_MODULE) $(GETDENTSTEST_MODULE) $(VFORKSTRESS_MODULE) $(EXECOK_MODULE) $(READBIG_MODULE) $(MMAPBIG_MODULE) $(MMAPHUGE_MODULE) $(EXECBIG_MODULE) $(PHDRSTRESS_MODULE) $(MUSL_HELLO_MODULE) $(DYN_HELLO_MODULE) $(FPUTEST_MODULE) $(IOVTEST_MODULE) $(FCHMODATTEST_MODULE) $(WAITPGIDTEST_MODULE) $(EXECLONGTEST_MODULE) $(AUXIDTEST_MODULE) $(PATHMAXTEST_MODULE) $(PATHERRTEST_MODULE) $(STATIDTEST_MODULE) $(FCNTLLOCKTEST_MODULE) $(SIGNALTEST_MODULE) $(FAULTTEST_MODULE) $(SYSRACETEST_MODULE) $(SCHEDSTRESS_MODULE) $(UPTIMETEST_MODULE) $(NETTEST_MODULE) $(NETDHCP_MODULE) $(BUSYBOX) $(MUSL_LDSO)
	BUSYBOX=$(BUSYBOX) MUSL_LDSO=$(MUSL_LDSO) MODULE_DIR=$(BUILD_DIR)/modules sh tools/build-synthetic-squashfs-rootfs.sh $@

$(ALPINE_SQUASHFS_IMAGE): $(LOGIN_MODULE) $(STATIDTEST_MODULE) $(NETDHCP_MODULE) tools/build-alpine-rootfs.sh tools/alpine-openrc-runlevels.policy modules/passwd modules/shadow modules/group
	ROOTFS_IMAGE_FORMAT=squashfs LOGIN_MODULE=$(LOGIN_MODULE) STATIDTEST_MODULE=$(STATIDTEST_MODULE) NETDHCP_MODULE=$(NETDHCP_MODULE) sh tools/build-alpine-rootfs.sh $@

$(ROOTFS_FLAVOR_STAMP): FORCE
	mkdir -p $(dir $@)
	@if ! test -f $@ || ! grep -qx '$(ROOTFS_FLAVOR)' $@; then \
		printf '%s\n' '$(ROOTFS_FLAVOR)' > $@; \
	fi

$(VIRTIO_BLK_TEST_IMAGE): $(BLOCK_IMAGE)
	mkdir -p $(dir $@)
	cp $(BLOCK_IMAGE) $@
	chmod u+w $@

$(EXT2_TEST_IMAGE): tools/build-ext2-test-image.sh
	sh tools/build-ext2-test-image.sh $@

$(EXT2_FSCK_TEST_IMAGE): $(EXT2_TEST_IMAGE) FORCE
	cp $< $@
	chmod u+w $@
	/sbin/e2fsck -fn $@ >/dev/null

$(GRUB_CFG): boot/grub.cfg FORCE
	mkdir -p $(BUILD_DIR)
	sed 's|@KERNEL_CMDLINE@|$(EFFECTIVE_KERNEL_CMDLINE)|g' $< > $@.tmp
	if ! cmp -s $@.tmp $@ 2>/dev/null; then mv $@.tmp $@; else rm $@.tmp; fi

$(GRUB_STANDALONE_CFG): boot/grub-standalone.cfg FORCE
	mkdir -p $(BUILD_DIR)
	sed 's|@KERNEL_CMDLINE@|$(EFFECTIVE_KERNEL_CMDLINE)|g' $< > $@.tmp
	if ! cmp -s $@.tmp $@ 2>/dev/null; then mv $@.tmp $@; else rm $@.tmp; fi

$(VIRTIO_BLK_TEST_GRUB_STANDALONE_CFG): boot/grub-standalone.cfg FORCE
	mkdir -p $(BUILD_DIR)
	sed -e 's|@KERNEL_CMDLINE@|$(VIRTIO_BLK_TEST_CMDLINE)|g' \
		-e '/virtio-bus\.server/a module2 /modules/virtio-blk.server virtio-blk' \
		$< > $@.tmp
	if ! cmp -s $@.tmp $@ 2>/dev/null; then mv $@.tmp $@; else rm $@.tmp; fi

$(VIRTIO_NET_TEST_GRUB_STANDALONE_CFG): boot/grub-standalone.cfg FORCE
	mkdir -p $(BUILD_DIR)
	sed -e 's|@KERNEL_CMDLINE@|$(VIRTIO_NET_TEST_CMDLINE)|g' \
		-e '/virtio-bus\.server/a module2 /modules/virtio-net.server virtio-net' \
		$< > $@.tmp
	if ! cmp -s $@.tmp $@ 2>/dev/null; then mv $@.tmp $@; else rm $@.tmp; fi

$(EXT2_TEST_GRUB_STANDALONE_CFG): boot/grub-standalone.cfg FORCE
	mkdir -p $(BUILD_DIR)
	sed -e 's|@KERNEL_CMDLINE@|$(EXT2_TEST_CMDLINE)|g' \
		-e '/squashfs\.server/a module2 /modules/ext2.server ext2' \
		-e '/disk0\.img/a module2 /modules/ext2-test.img ext2disk' \
		$< > $@.tmp
	if ! cmp -s $@.tmp $@ 2>/dev/null; then mv $@.tmp $@; else rm $@.tmp; fi

$(EXT2_FSCK_TEST_GRUB_STANDALONE_CFG): boot/grub-standalone.cfg FORCE
	mkdir -p $(BUILD_DIR)
	sed -e 's|@KERNEL_CMDLINE@|$(EXT2_FSCK_TEST_CMDLINE)|g' \
		-e '/squashfs\.server/a module2 /modules/ext2.server ext2' \
		-e '/virtio-bus\.server/a module2 /modules/virtio-blk.server virtio-blk' \
		$< > $@.tmp
	if ! cmp -s $@.tmp $@ 2>/dev/null; then mv $@.tmp $@; else rm $@.tmp; fi

$(EFI_BOOT_APP): $(KERNEL) $(GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(ESP_DIR)/EFI/BOOT $(ESP_DIR)/boot
	cp $(KERNEL) $(ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=$(GRUB_STANDALONE_CFG)" \
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
		"modules/sysfs.server=$(SYSFS_MODULE)" \
		"modules/utmpfs.server=$(UTMPFS_MODULE)" \
		"modules/squashfs.server=$(SQUASHFS_MODULE)" \
		"modules/unionfs.server=$(UNIONFS_MODULE)" \
		"modules/block.server=$(BLOCK_MODULE)" \
		"modules/pci.server=$(PCI_MODULE)" \
		"modules/usb-bus.server=$(USB_BUS_MODULE)" \
		"modules/usb-synth.server=$(USB_SYNTH_MODULE)" \
		"modules/xhci.server=$(XHCI_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(VIRTIO_BLK_TEST_EFI_BOOT_APP): $(KERNEL) $(VIRTIO_BLK_TEST_GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VIRTIO_BLK_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(VIRTIO_BLK_TEST_ESP_DIR)/EFI/BOOT $(VIRTIO_BLK_TEST_ESP_DIR)/boot
	cp $(KERNEL) $(VIRTIO_BLK_TEST_ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=$(VIRTIO_BLK_TEST_GRUB_STANDALONE_CFG)" \
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
		"modules/sysfs.server=$(SYSFS_MODULE)" \
		"modules/utmpfs.server=$(UTMPFS_MODULE)" \
		"modules/squashfs.server=$(SQUASHFS_MODULE)" \
		"modules/unionfs.server=$(UNIONFS_MODULE)" \
		"modules/block.server=$(BLOCK_MODULE)" \
		"modules/pci.server=$(PCI_MODULE)" \
		"modules/usb-bus.server=$(USB_BUS_MODULE)" \
		"modules/usb-synth.server=$(USB_SYNTH_MODULE)" \
		"modules/xhci.server=$(XHCI_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/virtio-blk.server=$(VIRTIO_BLK_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(VIRTIO_NET_TEST_EFI_BOOT_APP): $(KERNEL) $(VIRTIO_NET_TEST_GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VIRTIO_NET_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(VIRTIO_NET_TEST_ESP_DIR)/EFI/BOOT $(VIRTIO_NET_TEST_ESP_DIR)/boot
	cp $(KERNEL) $(VIRTIO_NET_TEST_ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=$(VIRTIO_NET_TEST_GRUB_STANDALONE_CFG)" \
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
		"modules/sysfs.server=$(SYSFS_MODULE)" \
		"modules/utmpfs.server=$(UTMPFS_MODULE)" \
		"modules/squashfs.server=$(SQUASHFS_MODULE)" \
		"modules/unionfs.server=$(UNIONFS_MODULE)" \
		"modules/block.server=$(BLOCK_MODULE)" \
		"modules/pci.server=$(PCI_MODULE)" \
		"modules/usb-bus.server=$(USB_BUS_MODULE)" \
		"modules/usb-synth.server=$(USB_SYNTH_MODULE)" \
		"modules/xhci.server=$(XHCI_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/virtio-net.server=$(VIRTIO_NET_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(EXT2_TEST_EFI_BOOT_APP): $(KERNEL) $(EXT2_TEST_GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(EXT2_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE) $(EXT2_TEST_IMAGE)
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(EXT2_TEST_ESP_DIR)/EFI/BOOT $(EXT2_TEST_ESP_DIR)/boot
	cp $(KERNEL) $(EXT2_TEST_ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=$(EXT2_TEST_GRUB_STANDALONE_CFG)" \
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
		"modules/sysfs.server=$(SYSFS_MODULE)" \
		"modules/utmpfs.server=$(UTMPFS_MODULE)" \
		"modules/squashfs.server=$(SQUASHFS_MODULE)" \
		"modules/unionfs.server=$(UNIONFS_MODULE)" \
		"modules/ext2.server=$(EXT2_MODULE)" \
		"modules/block.server=$(BLOCK_MODULE)" \
		"modules/pci.server=$(PCI_MODULE)" \
		"modules/usb-bus.server=$(USB_BUS_MODULE)" \
		"modules/usb-synth.server=$(USB_SYNTH_MODULE)" \
		"modules/xhci.server=$(XHCI_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/ext2-test.img=$(EXT2_TEST_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(EXT2_FSCK_TEST_EFI_BOOT_APP): $(KERNEL) $(EXT2_FSCK_TEST_GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(EXT2_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VIRTIO_BLK_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
	@if ! command -v $(GRUB_MKSTANDALONE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKSTANDALONE)"; exit 1; \
	fi
	mkdir -p $(EXT2_FSCK_TEST_ESP_DIR)/EFI/BOOT $(EXT2_FSCK_TEST_ESP_DIR)/boot
	cp $(KERNEL) $(EXT2_FSCK_TEST_ESP_DIR)/boot/bunixos.kernel
	$(GRUB_MKSTANDALONE) -O x86_64-efi -o $@ \
		"boot/grub/grub.cfg=$(EXT2_FSCK_TEST_GRUB_STANDALONE_CFG)" \
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
		"modules/sysfs.server=$(SYSFS_MODULE)" \
		"modules/utmpfs.server=$(UTMPFS_MODULE)" \
		"modules/squashfs.server=$(SQUASHFS_MODULE)" \
		"modules/unionfs.server=$(UNIONFS_MODULE)" \
		"modules/ext2.server=$(EXT2_MODULE)" \
		"modules/block.server=$(BLOCK_MODULE)" \
		"modules/pci.server=$(PCI_MODULE)" \
		"modules/usb-bus.server=$(USB_BUS_MODULE)" \
		"modules/usb-synth.server=$(USB_SYNTH_MODULE)" \
		"modules/xhci.server=$(XHCI_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/virtio-blk.server=$(VIRTIO_BLK_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(EFI_BOOT_IMG): $(KERNEL) $(GRUB_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
	@if ! command -v $(GRUB_MKRESCUE) >/dev/null 2>&1; then \
		echo "missing $(GRUB_MKRESCUE)"; exit 1; \
	fi
	@if ! command -v xorriso >/dev/null 2>&1; then \
		echo "missing xorriso, required by grub-mkrescue"; exit 1; \
	fi
	mkdir -p $(ISO_ROOT)/boot/grub
	mkdir -p $(ISO_ROOT)/modules
	cp $(KERNEL) $(ISO_ROOT)/boot/bunixos.kernel
	cp $(GRUB_CFG) $(ISO_ROOT)/boot/grub/grub.cfg
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
	cp $(SYSFS_MODULE) $(ISO_ROOT)/modules/sysfs.server
	cp $(UTMPFS_MODULE) $(ISO_ROOT)/modules/utmpfs.server
	cp $(SQUASHFS_MODULE) $(ISO_ROOT)/modules/squashfs.server
	cp $(UNIONFS_MODULE) $(ISO_ROOT)/modules/unionfs.server
	cp $(BLOCK_MODULE) $(ISO_ROOT)/modules/block.server
	cp $(PCI_MODULE) $(ISO_ROOT)/modules/pci.server
	cp $(USB_BUS_MODULE) $(ISO_ROOT)/modules/usb-bus.server
	cp $(USB_SYNTH_MODULE) $(ISO_ROOT)/modules/usb-synth.server
	cp $(XHCI_MODULE) $(ISO_ROOT)/modules/xhci.server
	cp $(VIRTIO_BUS_MODULE) $(ISO_ROOT)/modules/virtio-bus.server
	cp $(NET_MODULE) $(ISO_ROOT)/modules/net.server
	cp $(NETCFG_MODULE) $(ISO_ROOT)/modules/netcfg.server
	cp $(VFS_MODULE) $(ISO_ROOT)/modules/vfs.server
	cp $(PING_MODULE) $(ISO_ROOT)/modules/ping.server
	cp $(BLOCK_IMAGE) $(ISO_ROOT)/modules/disk0.img
	cp modules/vm.server $(ISO_ROOT)/modules/vm.server
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

RUN_ROOTFS_FLAVOR ?= alpine-squashfs
RUN_QEMU_NET_ARGS ?= $(QEMU_VIRTIO_NET_EXTERNAL_ARGS)

run:
	$(MAKE) ROOTFS_FLAVOR=$(RUN_ROOTFS_FLAVOR) run-alpine-net

run-alpine-net: $(VIRTIO_NET_TEST_EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(VIRTIO_NET_TEST_ESP_DIR) \
		$(RUN_QEMU_NET_ARGS) \
		-serial stdio -display none -no-reboot

run-virtio: $(EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		$(QEMU_VIRTIO_BLK_ARGS) \
		-serial stdio -display none -no-reboot

run-virtio-net: $(EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		$(QEMU_VIRTIO_NET_ARGS) \
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

run-riscv64-early: $(RISCV64_BOOTPKG)
	$(MAKE) ARCH=riscv64 all
	$(RISCV64_QEMU) -machine virt -m 128M -nographic -no-reboot \
		-kernel $(RISCV64_KERNEL) -initrd $(RISCV64_BOOTPKG)

test: test-parallel

test-alpine-rootfs: $(LOGIN_MODULE) $(STATIDTEST_MODULE) $(NETDHCP_MODULE) tools/build-alpine-rootfs.sh tools/alpine-openrc-runlevels.policy tools/test-alpine-rootfs.sh modules/passwd modules/shadow modules/group
	LOGIN_MODULE=$(LOGIN_MODULE) STATIDTEST_MODULE=$(STATIDTEST_MODULE) NETDHCP_MODULE=$(NETDHCP_MODULE) sh tools/test-alpine-rootfs.sh

test-boot: $(EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-squashfs.txt tools/test-boot-markers-squashfs-up.txt tools/test-boot-markers-alpine-smoke.txt tools/test-boot-markers-alpine-squashfs.txt
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) QEMU_TIMEOUT=$(QEMU_TIMEOUT) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log $(TEST_BOOT_MARKERS)

test-boot-riscv64-early: $(RISCV64_BOOTPKG)
	@command -v $(RISCV64_QEMU) >/dev/null 2>&1 || { echo "missing $(RISCV64_QEMU)"; exit 1; }
	$(MAKE) ARCH=riscv64 all
	mkdir -p $(BUILD_DIR)
	timeout 30s $(RISCV64_QEMU) -machine virt -m 128M -nographic \
		-no-reboot -kernel $(RISCV64_KERNEL) \
		-initrd $(RISCV64_BOOTPKG) > $(RISCV64_SERIAL_LOG)
	grep -aF "bunixos: riscv64 early bootstrap" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "timer: riscv64 tick" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "thread: riscv64 switch" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "pmm: riscv64 ranges" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "sched: riscv64 thread" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "vm: riscv64 hooks" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "syscall: riscv64 ecall" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "user: riscv64 mode" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "bootpkg: riscv64 initrd" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "cmdline: riscv64 bootpkg" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "module: riscv64 registered" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "module: riscv64 user elf" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "elf: riscv64 loader" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "copy: riscv64 user" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "bootstrap-riscv64: online" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "bootstrap-riscv64: abi-smoke launched" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "bootstrap-riscv64: linux launched" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "bootstrap-riscv64: musl-hello launched" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "linux-riscv64-server: online" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "linux-riscv64-server: write" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "linux-riscv64-server: exit_group" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "musl hello argc=1 argv0=/bin/musl-hello" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "native: riscv64 server argc=1 argv0=/bin/abi-smoke.user" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "native: riscv64 syscalls" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "bootstrap-riscv64: done" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "linux-riscv64-server: poweroff" $(RISCV64_SERIAL_LOG) >/dev/null
	grep -aF "machine: poweroff" $(RISCV64_SERIAL_LOG) >/dev/null

test-boot-ext2: $(EXT2_TEST_EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-ext2.txt
	ESP_DIR=$(EXT2_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-ext2.txt

test-boot-ext2-root: EXT2_TEST_CMDLINE=log=info ext2-root-test
test-boot-ext2-root: $(EXT2_TEST_EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-ext2-root.txt
	ESP_DIR=$(EXT2_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="machine: poweroff" \
		sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-ext2-root.txt

test-boot-usb-synth: KERNEL_CMDLINE=log=info usb-synth-test
test-boot-usb-synth: $(EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-usb-synth.txt
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="machine: poweroff" \
		sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-usb-synth.txt

test-boot-xhci-discovery: KERNEL_CMDLINE=log=info xhci-test
test-boot-xhci-discovery: $(EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-xhci-discovery.txt
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="machine: poweroff" \
		QEMU_EXTRA_ARGS="$(QEMU_XHCI_ARGS)" sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-xhci-discovery.txt

test-boot-usb:
	$(MAKE) test-boot-usb-synth
	$(MAKE) test-boot-xhci-discovery

test-boot-ext2-fsck: $(EXT2_FSCK_TEST_EFI_BOOT_APP) $(EXT2_FSCK_TEST_IMAGE) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-ext2-fsck.txt
	ESP_DIR=$(EXT2_FSCK_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="machine: poweroff" \
		QEMU_EXTRA_ARGS="$(QEMU_EXT2_FSCK_TEST_ARGS)" sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-ext2-fsck.txt
	grep -aF "virtio-blk: block online" $(BUILD_DIR)/serial.log >/dev/null
	/sbin/e2fsck -fn $(EXT2_FSCK_TEST_IMAGE) >/dev/null

test-boot-virtio: $(EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-squashfs.txt
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_BLK_ARGS)" sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log $(TEST_BOOT_MARKERS)
	grep -aF "pci: online" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "pci: function index=0" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "vendor=6900 device=4162" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "pci: ready functions=" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-bus: device index=0" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-bus: features index=0" $(BUILD_DIR)/serial.log >/dev/null
	! grep -aF "virtio-bus: features index=0 device=0 " $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-bus: ready devices=1" $(BUILD_DIR)/serial.log >/dev/null

test-boot-virtio-net: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-squashfs.txt
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=$(QEMU_TIMEOUT) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_ARGS)" sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log $(TEST_BOOT_MARKERS)
	grep -aF "pci: online" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "pci: function index=0" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "vendor=6900 device=4161" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "pci: ready functions=" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-bus: device index=0 vendor=6900 device=4161" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-bus: features index=0" $(BUILD_DIR)/serial.log >/dev/null
	! grep -aF "virtio-bus: features index=0 device=0 " $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-bus: ready devices=1" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-net: reset" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-net: negotiated" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-net: queues ready" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-net: attached iface=" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-net: rx ready" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-net: tx ready" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "netcfg: dhcp fallback lease installed" $(BUILD_DIR)/serial.log >/dev/null

test-boot-virtio-net-dhcp: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=90s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_DHCP_OK \
		BUNIX_CMD='udhcpc -n -q -i eth0 -t 1 -T 1; cat /proc/net/config; busybox grep -F "iface eth0" /proc/net/config; busybox grep -F "rxq 0 txq 0" /proc/net/config' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-ifup:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-ifup-run

test-boot-virtio-net-ifup-run: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=120s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_IFUP_OK \
		BUNIX_CMD='/sbin/ifdown eth0; /sbin/ifup eth0; cat /proc/net/config; busybox grep -F "iface eth0" /proc/net/config; busybox grep -F "default_ipv4 1" /proc/net/config; busybox grep -F "rxq 0 txq 0" /proc/net/config' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-networking:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-networking-run

test-boot-virtio-net-networking-run: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=180s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_NETWORKING_OK \
		BUNIX_CMD='C=/proc/net/config; rc-service networking status; rc-service networking restart; test -e /run/openrc/started/networking; cat $$C; grep -F "iface eth0" $$C; grep -F "default_ipv4 1" $$C; grep -F "rxq 0 txq 0" $$C; ping -c1 -W4 10.0.2.2; ping -c1 -W4 4.2.2.1; ping -c1 -W4 8.8.8.8' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-socket-peer: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/virtio-net-peer.py tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=120s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_SOCKET_PEER_OK \
		BUNIX_TEST_SIDECAR_READY_FILE='$(BUILD_DIR)/virtio-net-peer.ready' \
		BUNIX_TEST_SIDECAR_CMD='rm -f $(BUILD_DIR)/virtio-net-peer.ready; python3 tools/virtio-net-peer.py --mcast $(QEMU_VIRTIO_NET_SOCKET_MCAST) --ready-file $(BUILD_DIR)/virtio-net-peer.ready --duration 120 --verbose' \
		BUNIX_CMD='cat /proc/net/config; busybox grep -F "iface eth0" /proc/net/config; busybox grep -F "default_ipv4 1" /proc/net/config; busybox ping -c 1 -W 4 10.0.2.2; cat /proc/net/dev; set -- $$(busybox grep -F "eth0:" /proc/net/dev); test "$$2" != 0; test "$$10" != 0' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_SOCKET_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-external-ping:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-external-ping-run

test-boot-virtio-net-external-ping-run: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=180s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_EXTERNAL_PING_OK \
		BUNIX_CMD='udhcpc -n -q -i eth0 -t 1 -T 1; C=/proc/net/config; grep -F "iface eth0" $$C; grep -F "default_ipv4 1" $$C; ping -c4 -W4 4.2.2.1 >/tmp/p; cat /tmp/p; grep -F "64 bytes from 4.2.2.1:" /tmp/p; grep -F "4 packets transmitted, 4 packets received" /tmp/p; ping -c1 -W4 8.8.8.8' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" sh tools/test-command.sh

test-boot-virtio-blk: $(VIRTIO_BLK_TEST_EFI_BOOT_APP) $(VIRTIO_BLK_TEST_IMAGE) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-squashfs.txt
	ESP_DIR=$(VIRTIO_BLK_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_BLK_TEST_ARGS)" sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log $(TEST_BOOT_MARKERS)
	grep -aF "pci: online" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "pci: function index=0" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "vendor=6900 device=4162" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "pci: ready functions=" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "bootstrap: virtio-blk test" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-blk: feature fail ok" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-blk: read ok" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-blk: write ok" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "virtio-blk: flush ok" $(BUILD_DIR)/serial.log >/dev/null

test-boot-virtio-blk-irq: test-boot-virtio-blk
	grep -aF "virtio-blk: irq ready" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "irq: bind" $(BUILD_DIR)/serial.log >/dev/null

test-boot-virtio-blk-backend:
	$(MAKE) VIRTIO_BLK_TEST_CMDLINE="log=info virtio-blk-test virtio-blk-block-test" QEMU_TIMEOUT=120s TEST_BOOT_MARKERS=tools/test-boot-markers-virtio-blk-backend.txt test-boot-virtio-blk
	grep -aF "virtio-blk: block online" $(BUILD_DIR)/serial.log >/dev/null

test-boot-virtio-blk-irq-backend: test-boot-virtio-blk-backend
	grep -aF "virtio-blk: irq ready" $(BUILD_DIR)/serial.log >/dev/null
	grep -aF "irq: bind" $(BUILD_DIR)/serial.log >/dev/null

test-shell: $(EFI_BOOT_APP)
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		sh tools/test-shell.sh

test-shell-part: $(EFI_BOOT_APP)
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		BUNIX_SHELL_PART="$(BUNIX_SHELL_PART)" sh tools/test-shell.sh

test-shell-squashfs-rootfs:
	$(MAKE) ROOTFS_FLAVOR=squashfs BUNIX_SHELL_PART=rootfs-vfs-proc-dev test-shell-part

test-smoke: $(EFI_BOOT_APP)
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		BUNIX_SHELL_PART=smoke sh tools/test-shell.sh

test-smoke-parallel: $(EFI_BOOT_APP)
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) \
		BUNIX_TEST_SET=smoke BUNIX_TEST_JOBS="$(BUNIX_TEST_JOBS)" \
		sh tools/test-parallel.sh

test-shell-parallel: $(EFI_BOOT_APP) $(PARALLEL_ALPINE_ESP)
	ESP_DIR=$(ESP_DIR) ALPINE_ESP_DIR=$(ALPINE_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) \
		BUNIX_TEST_SET="$(PARALLEL_TEST_SET)" \
		BUNIX_TEST_JOBS="$(BUNIX_TEST_JOBS)" \
		sh tools/test-parallel.sh

test-parallel: test-shell-parallel

test-prune-artifacts:
	sh tools/test-prune-artifacts.sh

test-command: $(EFI_BOOT_APP)
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		sh tools/test-command.sh

test-shell-dynamic:
	$(MAKE) -B test-shell BUSYBOX=$(BUSYBOX_DYNAMIC)

test-shell-static:
	$(MAKE) -B test-shell BUSYBOX=$(BUSYBOX_STATIC)

list-shell-shards:
	sh tools/list-shell-shards.sh

audit-linux-syscalls:
	sh tools/audit-linux-syscalls.sh

security-audit-check:
	sh tools/check-security-explorations.sh

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
