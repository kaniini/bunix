ARCH ?= x86_64
HOSTCC ?= cc
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
RISCV64_EARLY_SERIAL_LOG_DEFAULT := $(BUILD_DIR)/riscv64/test-boot-early.serial.log
RISCV64_ALPINE_SERIAL_LOG_DEFAULT := $(BUILD_DIR)/riscv64/test-boot-alpine.serial.log
RISCV64_UART_SERIAL_LOG_DEFAULT := $(BUILD_DIR)/riscv64/test-boot-uart-console.serial.log
RISCV64_EARLY_SERIAL_LOG ?= $(RISCV64_EARLY_SERIAL_LOG_DEFAULT)
RISCV64_ALPINE_SERIAL_LOG ?= $(RISCV64_ALPINE_SERIAL_LOG_DEFAULT)
RISCV64_UART_SERIAL_LOG ?= $(RISCV64_UART_SERIAL_LOG_DEFAULT)
RISCV64_QEMU ?= qemu-system-riscv64
RISCV64_CC ?= clang
RISCV64_CC_TARGET_FLAGS ?= --target=riscv64-alpine-linux-musl
RISCV64_LD ?= riscv64-alpine-linux-musl-ld
RISCV64_OBJDUMP ?= llvm-objdump
RISCV64_READELF ?= llvm-readelf
RISCV64_USER_ABI_MODULE := $(BUILD_DIR)/riscv64/modules/abi-smoke.user
RISCV64_BOOTPKG := $(BUILD_DIR)/riscv64/bootpkg.img
RISCV64_BOOTPKG_MULTI := $(BUILD_DIR)/riscv64/bootpkg-multi.img
RISCV64_SLEEP_BOOTPKG := $(BUILD_DIR)/riscv64/bootpkg-sleep.img
RISCV64_ALPINE_BOOTPKG := $(BUILD_DIR)/riscv64/bootpkg-alpine.img
RISCV64_UART_BOOTPKG := $(BUILD_DIR)/riscv64/bootpkg-uart.img
RISCV64_KERNEL_CMDLINE ?= log=info riscv64-bootpkg-test riscv64-diag riscv64-selftest
RISCV64_SLEEP_KERNEL_CMDLINE ?= log=info riscv64-bootpkg-test riscv64-sleep-smoke
RISCV64_ALPINE_KERNEL_CMDLINE ?= log=info riscv64-bootpkg-test riscv64-alpine-test
RISCV64_UART_KERNEL_CMDLINE ?= log=info riscv64-bootpkg-test riscv64-uart-console riscv64-diag riscv64-selftest
RISCV64_MUSLCC_PREFIX ?= $(BUILD_DIR)/toolchains/riscv64-linux-musl-cross
RISCV64_MUSLCC_GCC := $(RISCV64_MUSLCC_PREFIX)/bin/riscv64-linux-musl-gcc
RISCV64_MUSLCC_SYSROOT := $(RISCV64_MUSLCC_PREFIX)/riscv64-linux-musl
RISCV64_MUSLCC_GCC_LIBDIR := $(RISCV64_MUSLCC_PREFIX)/lib/gcc/riscv64-linux-musl/11.2.1
RISCV64_MUSL_LDSO_SOURCE := $(RISCV64_MUSLCC_SYSROOT)/lib/libc.so
RISCV64_MUSL_HELLO_MODULE := $(BUILD_DIR)/riscv64/modules/musl-hello.user
RISCV64_SYSCALL_SMOKE_MODULE := $(BUILD_DIR)/riscv64/modules/rv64-syscall-smoke.user
RISCV64_USERMEMTEST_MODULE := $(BUILD_DIR)/riscv64/modules/usermemtest.user
RISCV64_SLEEP_SMOKE_MODULE := $(BUILD_DIR)/riscv64/modules/sleep-smoke.user
RISCV64_DYN_HELLO_MODULE := $(BUILD_DIR)/riscv64/modules/dyn-hello.user
RISCV64_MUSL_LDSO := $(BUILD_DIR)/riscv64/modules/ld-musl-riscv64.so.1
RISCV64_SHARED_LINUX_SERVER_MODULE := $(BUILD_DIR)/riscv64/modules/linux-shared.server
RISCV64_NAMES_MODULE := $(BUILD_DIR)/riscv64/modules/names.server
RISCV64_TIME_MODULE := $(BUILD_DIR)/riscv64/modules/time.server
RISCV64_USER_MODULE := $(BUILD_DIR)/riscv64/modules/user.server
RISCV64_PROC_MODULE := $(BUILD_DIR)/riscv64/modules/proc.server
RISCV64_BLOCK_MODULE := $(BUILD_DIR)/riscv64/modules/block.server
RISCV64_VFS_MODULE := $(BUILD_DIR)/riscv64/modules/vfs.server
RISCV64_TMPFS_MODULE := $(BUILD_DIR)/riscv64/modules/tmpfs.server
RISCV64_SQUASHFS_MODULE := $(BUILD_DIR)/riscv64/modules/squashfs.server
RISCV64_UNIONFS_MODULE := $(BUILD_DIR)/riscv64/modules/unionfs.server
RISCV64_BOOTSTRAP_MODULE := $(BUILD_DIR)/riscv64/modules/bootstrap.server
RISCV64_SMOKE_SQUASHFS_IMAGE := $(BUILD_DIR)/riscv64/disk0.sqfs
RISCV64_ALPINE_SQUASHFS_IMAGE := $(BUILD_DIR)/riscv64/alpine-rootfs.sqfs
RISCV64_USER_CRT0_OBJ := $(BUILD_DIR)/riscv64/user/crt0-riscv64.S.o
RISCV64_USER_STRING_OBJ := $(BUILD_DIR)/riscv64/user/runtime/string.c.o
RISCV64_USER_RUNTIME_OBJS := $(RISCV64_USER_STRING_OBJ)
RISCV64_BOOTPKG_SERVER_ARGS := \
	$(RISCV64_NAMES_MODULE) names \
	$(RISCV64_TIME_MODULE) time \
	$(RISCV64_USER_MODULE) user \
	$(RISCV64_PROC_MODULE) proc \
	$(RISCV64_BLOCK_MODULE) block \
	$(RISCV64_VFS_MODULE) vfs \
	$(RISCV64_SQUASHFS_MODULE) squashfs \
	$(RISCV64_BOOTSTRAP_MODULE) bootstrap \
	$(RISCV64_USER_ABI_MODULE) abi-smoke.user \
	$(RISCV64_SLEEP_SMOKE_MODULE) sleep-smoke.user \
	$(RISCV64_SHARED_LINUX_SERVER_MODULE) linux
RISCV64_BOOTPKG_SMOKE_ARGS := \
	$(RISCV64_SMOKE_SQUASHFS_IMAGE) disk0 \
	$(RISCV64_BOOTPKG_SERVER_ARGS)
RISCV64_BOOTPKG_ALPINE_ARGS := \
	$(RISCV64_ALPINE_SQUASHFS_IMAGE) disk0 \
	$(RISCV64_BOOTPKG_SERVER_ARGS)
RISCV64_BOOTPKG_UART_ARGS := \
	$(RISCV64_NAMES_MODULE) names \
	$(RISCV64_BOOTSTRAP_MODULE) bootstrap \
	$(RISCV64_USER_ABI_MODULE) abi-smoke.user \
	$(RISCV64_SLEEP_SMOKE_MODULE) sleep-smoke.user
BOOTSTRAP_MODULE := $(BUILD_DIR)/modules/bootstrap.server
USER_CRT0_OBJ := $(BUILD_DIR)/user/crt0.S.o
BOOTSTRAP_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/bootstrap/main.c.o
CONSOLE_MODULE := $(BUILD_DIR)/modules/console.server
CONSOLE_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/console/main.c.o
NAMES_MODULE := $(BUILD_DIR)/modules/names.server
NAMES_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/names/main.c.o
NAMES_TEST_MODULE := $(BUILD_DIR)/modules/names-test.server
NAMES_TEST_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/names-test/main.c.o
MGMT_TEST_MODULE := $(BUILD_DIR)/modules/mgmt-test.server
MGMT_TEST_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/mgmt-test/main.c.o
TIME_MODULE := $(BUILD_DIR)/modules/time.server
TIME_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/time/main.c.o
SCHED_MODULE := $(BUILD_DIR)/modules/sched.server
SCHED_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/sched/main.c.o
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
INPUT_MODULE := $(BUILD_DIR)/modules/input.server
INPUT_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/input/main.c.o
USB_HID_KBD_MODULE := $(BUILD_DIR)/modules/usb-hid-kbd.server
USB_HID_KBD_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/usb-hid-kbd/main.c.o
USB_STORAGE_MODULE := $(BUILD_DIR)/modules/usb-storage.server
USB_STORAGE_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/usb-storage/main.c.o
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
LIFECYCLETEST_MODULE := $(BUILD_DIR)/modules/lifecycletest.user
LIFECYCLETEST_MODULE_OBJS := $(BUILD_DIR)/user/lifecycletest/main.S.o
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
USERMEMTEST_MODULE := $(BUILD_DIR)/modules/usermemtest.user
FCHMODATTEST_MODULE := $(BUILD_DIR)/modules/fchmodattest.user
WAITPGIDTEST_MODULE := $(BUILD_DIR)/modules/waitpgidtest.user
EXECLONGTEST_MODULE := $(BUILD_DIR)/modules/execlongtest.user
AUXIDTEST_MODULE := $(BUILD_DIR)/modules/auxidtest.user
PATHMAXTEST_MODULE := $(BUILD_DIR)/modules/pathmaxtest.user
PATHERRTEST_MODULE := $(BUILD_DIR)/modules/patherrtest.user
STATIDTEST_MODULE := $(BUILD_DIR)/modules/statidtest.user
LDSOPATHTEST_MODULE := $(BUILD_DIR)/modules/ldsopathtest.user
FCNTLLOCKTEST_MODULE := $(BUILD_DIR)/modules/fcntllocktest.user
OPENRCTEST_MODULE := $(BUILD_DIR)/modules/openrctest.user
PROCFSPIPETEST_MODULE := $(BUILD_DIR)/modules/procfspipetest.user
SIGNALTEST_MODULE := $(BUILD_DIR)/modules/signaltest.user
TTYTEST_MODULE := $(BUILD_DIR)/modules/ttytest.user
TTYSIGTEST_MODULE := $(BUILD_DIR)/modules/ttysigtest.user
FAULTTEST_MODULE := $(BUILD_DIR)/modules/faulttest.user
FAULTTEST_MODULE_OBJS := $(BUILD_DIR)/user/faulttest/main.S.o
LOWMEMTEST_MODULE := $(BUILD_DIR)/modules/lowmemtest.user
LOWMEMTEST_MODULE_OBJS := $(BUILD_DIR)/user/lowmemtest/main.S.o
SYSRACETEST_MODULE := $(BUILD_DIR)/modules/sysracetest.user
SCHEDSTRESS_MODULE := $(BUILD_DIR)/modules/schedstress.user
SCHEDBENCH_MODULE := $(BUILD_DIR)/modules/schedbench.user
UPTIMETEST_MODULE := $(BUILD_DIR)/modules/uptimetest.user
NETTEST_MODULE := $(BUILD_DIR)/modules/nettest.user
NETDHCP_MODULE := $(BUILD_DIR)/modules/bunix-udhcpc-script.user
NETDHCP_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/netdhcp/main.c.o
PRIVTEST_MODULE := $(BUILD_DIR)/modules/privtest.user
DYN_HELLO_MODULE := $(BUILD_DIR)/modules/dyn-hello.user
BUSYBOX_DYNAMIC ?= /bin/busybox
BUSYBOX_STATIC ?= /usr/bin/busybox.static
BUSYBOX ?= $(BUSYBOX_DYNAMIC)
MUSL_LDSO ?= /lib/ld-musl-x86_64.so.1
PING_MODULE := $(BUILD_DIR)/modules/ping.server
PING_MODULE_OBJS := $(USER_CRT0_OBJ) $(BUILD_DIR)/user/ping/main.c.o
X86_USER_LD_MODULES := \
	BOOTSTRAP_MODULE \
	ALLOCTEST_MODULE \
	CONSOLE_MODULE \
	NAMES_MODULE \
	NAMES_TEST_MODULE \
	MGMT_TEST_MODULE \
	TIME_MODULE \
	SCHED_MODULE \
	USER_MODULE \
	LINUX_SERVER_MODULE \
	PROC_MODULE \
	PROCFS_MODULE \
	TMPFS_MODULE \
	DEVFS_MODULE \
	SYSFS_MODULE \
	UTMPFS_MODULE \
	UNIONFS_MODULE \
	EXT2_MODULE \
	SQUASHFS_MODULE \
	BLOCK_MODULE \
	PCI_MODULE \
	USB_BUS_MODULE \
	USB_SYNTH_MODULE \
	XHCI_MODULE \
	INPUT_MODULE \
	USB_HID_KBD_MODULE \
	USB_STORAGE_MODULE \
	VIRTIO_BUS_MODULE \
	NET_MODULE \
	NETCFG_MODULE \
	VIRTIO_BLK_MODULE \
	VIRTIO_NET_MODULE \
	VFS_MODULE \
	FIRST_MODULE \
	IPCSTRESS_MODULE \
	LOGIN_MODULE \
	NETDHCP_MODULE \
	LXTEST_MODULE \
	GETDENTSTEST_MODULE \
	VFORKSTRESS_MODULE \
	LIFECYCLETEST_MODULE \
	EXECOK_MODULE \
	READBIG_MODULE \
	MMAPBIG_MODULE \
	MMAPHUGE_MODULE \
	FAULTTEST_MODULE \
	LOWMEMTEST_MODULE \
	PING_MODULE
SYNTHETIC_SQUASHFS_IMAGE := $(BUILD_DIR)/modules/disk0.sqfs
ALPINE_SQUASHFS_IMAGE := $(BUILD_DIR)/modules/alpine-disk0.sqfs
ROOTFS_FLAVOR ?= squashfs
BLOCK_IMAGE := $(if $(filter alpine-squashfs,$(ROOTFS_FLAVOR)),$(ALPINE_SQUASHFS_IMAGE),$(SYNTHETIC_SQUASHFS_IMAGE))
VIRTIO_BLOCK_IMAGE ?= $(BLOCK_IMAGE)
VIRTIO_BLK_TEST_IMAGE := $(BUILD_DIR)/virtio-blk-test.img
EXT2_TEST_IMAGE := $(BUILD_DIR)/modules/ext2-test.img
EXT2_FSCK_TEST_IMAGE := $(BUILD_DIR)/ext2-fsck-test.img
USB_STORAGE_TEST_IMAGE := $(BUILD_DIR)/usb-storage-test.img
QEMU_VIRTIO_BLK_ARGS := -drive if=none,id=bunix-virtio0,format=raw,readonly=on,file=$(VIRTIO_BLOCK_IMAGE) -device virtio-blk-pci,disable-legacy=on,drive=bunix-virtio0,bus=pcie.0,addr=0x6
QEMU_VIRTIO_BLK_TEST_ARGS := -drive if=none,id=bunix-virtio0,format=raw,file=$(VIRTIO_BLK_TEST_IMAGE) -device virtio-blk-pci,disable-legacy=on,drive=bunix-virtio0,bus=pcie.0,addr=0x6
QEMU_EXT2_FSCK_TEST_ARGS := -drive if=none,id=bunix-virtio0,format=raw,file=$(EXT2_FSCK_TEST_IMAGE) -device virtio-blk-pci,disable-legacy=on,drive=bunix-virtio0,bus=pcie.0,addr=0x6
QEMU_VIRTIO_NET_ARGS := -netdev user,id=bunix-net0,restrict=on -device virtio-net-pci,disable-legacy=on,netdev=bunix-net0,mac=52:54:00:18:00:01,bus=pcie.0,addr=0x7
QEMU_VIRTIO_NET_EXTERNAL_ARGS := -netdev user,id=bunix-net0 -device virtio-net-pci,disable-legacy=on,netdev=bunix-net0,mac=52:54:00:18:00:01,bus=pcie.0,addr=0x7
QEMU_VIRTIO_NET_SOCKET_MCAST ?= 230.18.0.1:18100
QEMU_VIRTIO_NET_SOCKET_ARGS := -netdev socket,id=bunix-net0,mcast=$(QEMU_VIRTIO_NET_SOCKET_MCAST) -device virtio-net-pci,disable-legacy=on,netdev=bunix-net0,mac=52:54:00:18:00:01,bus=pcie.0,addr=0x7
QEMU_XHCI_ARGS := -device qemu-xhci,id=bunix-xhci,bus=pcie.0,addr=0x8 -device usb-kbd,bus=bunix-xhci.0
QEMU_XHCI_STORAGE_ARGS := -drive if=none,id=bunix-usb-storage0,format=raw,file=$(USB_STORAGE_TEST_IMAGE) -device qemu-xhci,id=bunix-xhci,bus=pcie.0,addr=0x8 -device usb-storage,drive=bunix-usb-storage0,bus=bunix-xhci.0
QEMU_TIMEOUT ?= $(if $(filter alpine-squashfs,$(ROOTFS_FLAVOR)),120s,60s)
TEST_BOOT_MARKERS := $(if $(filter alpine-squashfs,$(ROOTFS_FLAVOR)),tools/test-boot-markers-alpine-squashfs.txt,tools/test-boot-markers-squashfs.txt)
ROOTFS_FLAVOR_STAMP := $(BUILD_DIR)/rootfs-flavor.stamp
PARALLEL_TEST_SET := $(if $(BUNIX_TEST_SET),$(BUNIX_TEST_SET),all)
PARALLEL_ALPINE_SELECTED := $(or $(filter all openrc,$(PARALLEL_TEST_SET)),$(findstring alpine-openrc,$(PARALLEL_TEST_SET)))
PARALLEL_ALPINE_ESP := $(if $(PARALLEL_ALPINE_SELECTED),$(ALPINE_EFI_BOOT_APP))
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
ROOTFS_ROOT_MOUNT_SOAK := modules/root-mount-soak.sh
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
	kernel/boot_timing.c \
	kernel/bootpkg.c \
	kernel/buffer.c \
	kernel/cmdline.c \
	kernel/console.c \
	kernel/elf.c \
	kernel/ipc.c \
	kernel/multiboot2.c \
	kernel/name.c \
	kernel/pmm.c \
	kernel/runtime.c \
	kernel/pmm_multiboot2.c \
	kernel/sched.c \
	kernel/slab.c \
	kernel/server.c \
	kernel/server_multiboot2.c \
	kernel/spinlock.c \
	kernel/string.c \
	kernel/task_lifecycle.c \
	kernel/timer.c \
	kernel/vm.c \
	servers/vm/vm.c
KERNEL_GENERIC_SRCS_riscv64 := \
	kernel/bootpkg.c \
	kernel/boot_timing.c \
	kernel/buffer.c \
	kernel/cmdline.c \
	kernel/elf.c \
	kernel/ipc.c \
	kernel/name.c \
	kernel/pmm.c \
	kernel/runtime.c \
	kernel/sched.c \
	kernel/slab.c \
	kernel/server.c \
	kernel/spinlock.c \
	kernel/string.c \
	kernel/task_lifecycle.c \
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
	$(BUILD_DIR)/user/names-test/main.c.o \
	$(BUILD_DIR)/user/mgmt-test/main.c.o \
	$(BUILD_DIR)/user/time/main.c.o \
	$(BUILD_DIR)/user/sched/main.c.o \
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
	$(BUILD_DIR)/user/input/main.c.o \
	$(BUILD_DIR)/user/usb-hid-kbd/main.c.o \
	$(BUILD_DIR)/user/usb-storage/main.c.o \
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
	$(BUILD_DIR)/user/lifecycletest/main.S.o \
	$(BUILD_DIR)/user/execok/main.S.o \
	$(BUILD_DIR)/user/readbig/main.S.o \
	$(BUILD_DIR)/user/mmapbig/main.S.o \
	$(BUILD_DIR)/user/phdrstress/main.c.o \
	$(BUILD_DIR)/user/ping/main.c.o
DEPS := $(KERNEL_OBJS:.o=.d) $(USER_OBJS:.o=.d)

.PHONY: all clean run run-fast run-fast-qemu run-profile run-profile-fast run-profile-alpine run-profile-alpine-net test-run-fast test-run-alpine test-run-alpine-net test-run-profile-log-volume run-alpine run-alpine-qemu run-alpine-net run-alpine-net-qemu run-virtio run-virtio-net run-kernel run-iso run-riscv64-early run-riscv64-alpine riscv64-muslcc-toolchain test test-alpine-rootfs test-alpine-rootfs-stock-networking test-riscv64-alpine-rootfs test-riscv64-dynamic-linker-artifacts test-linux-exec-abi test-proc-exec-stack test-path-safety test-boot test-boot-alpine-stock-networking test-boot-openrc-fork-reclaim test-boot-openrc-fork-reclaim-run test-boot-net-route test-boot-handle-race test-linux-lifecycle test-lowmem-isolation test-user-memory-contract test-boot-ext2 test-boot-ext2-fsck test-boot-ext2-root test-boot-riscv64-early test-boot-riscv64-sleep test-boot-riscv64-alpine test-boot-riscv64-uart-console test-riscv64-log-isolation test-riscv64-bootpkg test-riscv64-shared-linux-server-build test-riscv64-proc-server-build test-riscv64-fs-server-build test-riscv64-user-abi test-boot-usb test-boot-usb-synth test-boot-xhci-discovery test-boot-usb-hid-kbd test-boot-usb-storage test-boot-virtio test-boot-virtio-net test-boot-virtio-net-dhcp test-boot-virtio-net-ifup test-boot-virtio-net-ifup-run test-boot-virtio-net-networking test-boot-virtio-net-networking-run test-boot-virtio-net-external-stack test-boot-virtio-net-external-ping-strict test-boot-virtio-net-external-ping-strict-run test-boot-virtio-net-dns-wget test-boot-virtio-net-dns-wget-run test-boot-virtio-net-apk-add test-boot-virtio-net-apk-add-run test-boot-virtio-net-wget-readv test-boot-virtio-net-wget-readv-run test-boot-virtio-net-socket-peer test-boot-virtio-net-socket-peer-ipv6 test-boot-virtio-net-socket-peer-udp6 test-boot-virtio-net-socket-peer-tcp6 test-boot-virtio-net-external-ping test-boot-virtio-net-external-ping-run test-boot-virtio-blk test-boot-virtio-blk-irq test-boot-virtio-blk-backend test-boot-virtio-blk-irq-backend test-command test-shell test-shell-part test-shell-squashfs-rootfs test-smoke test-smoke-parallel test-shell-parallel test-parallel test-prune-artifacts test-shell-static test-shell-dynamic list-shell-shards audit-linux-syscalls security-audit-check iso esp check-tools FORCE

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

$(BUILD_DIR)/user/linux/main.c.o: user/linux/exec_abi.c
$(BUILD_DIR)/user/proc/main.c.o: user/proc/exec_stack.c

$(RISCV64_USER_CRT0_OBJ): user/crt0-riscv64.S user/include/bunix/syscall.h Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-g -ffreestanding -fno-pic -fno-pie -Iuser/include -MMD -MP \
		-c user/crt0-riscv64.S -o $@

$(RISCV64_USER_ABI_MODULE): $(RISCV64_USER_CRT0_OBJ) user/riscv64-abi/main.c user/user.ld user/include/bunix/syscall.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include \
		-Wall -Wextra -Werror -MMD -MP \
		-c user/riscv64-abi/main.c -o $(BUILD_DIR)/riscv64/user/riscv64-abi.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/riscv64-abi.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)

$(RISCV64_SLEEP_SMOKE_MODULE): $(RISCV64_USER_CRT0_OBJ) user/riscv64-sleep-smoke/main.c user/user.ld user/include/bunix/syscall.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include \
		-Wall -Wextra -Werror -MMD -MP \
		-c user/riscv64-sleep-smoke/main.c -o $(BUILD_DIR)/riscv64/user/riscv64-sleep-smoke.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/riscv64-sleep-smoke.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

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

$(RISCV64_SYSCALL_SMOKE_MODULE): user/riscv64-syscall-smoke/main.c $(RISCV64_MUSLCC_GCC) Makefile
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

$(RISCV64_USERMEMTEST_MODULE): user/usermemtest/main.c $(RISCV64_MUSLCC_GCC) Makefile
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

$(RISCV64_DYN_HELLO_MODULE): user/musl-hello/main.c $(RISCV64_MUSLCC_GCC) Makefile
	mkdir -p $(dir $@)
	$(RISCV64_CC) --target=riscv64-linux-musl -march=rv64gc -mabi=lp64d \
		-isystem $(RISCV64_MUSLCC_SYSROOT)/include \
		-fuse-ld=/usr/bin/$(RISCV64_LD) \
		-B $(RISCV64_MUSLCC_SYSROOT)/lib \
		-B $(RISCV64_MUSLCC_GCC_LIBDIR) \
		-L $(RISCV64_MUSLCC_SYSROOT)/lib \
		-L $(RISCV64_MUSLCC_GCC_LIBDIR) \
		-Wl,--dynamic-linker=/lib/ld-musl-riscv64.so.1 \
		-O2 -g $< -o $@
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null
	$(RISCV64_READELF) -l $@ | grep -F "/lib/ld-musl-riscv64.so.1" >/dev/null

$(RISCV64_MUSL_LDSO): $(RISCV64_MUSLCC_GCC)
	mkdir -p $(dir $@)
	cp $(RISCV64_MUSL_LDSO_SOURCE) $@
	chmod 0555 $@

$(RISCV64_SHARED_LINUX_SERVER_MODULE): $(RISCV64_USER_CRT0_OBJ) user/linux/main.c user/linux/exec_abi.c user/user.ld user/include/bunix/syscall.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/linux/main.c -o $(BUILD_DIR)/riscv64/user/linux-shared.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/linux-shared.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_NAMES_MODULE): $(RISCV64_USER_CRT0_OBJ) user/names/main.c user/user.ld user/include/bunix/syscall.h user/include/bunix/alloc.h user/include/bunix/id_table.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/names/main.c -o $(BUILD_DIR)/riscv64/user/names.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/names.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_TIME_MODULE): $(RISCV64_USER_CRT0_OBJ) user/time/main.c user/user.ld user/include/bunix/syscall.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/time/main.c -o $(BUILD_DIR)/riscv64/user/time.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/time.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_USER_MODULE): $(RISCV64_USER_CRT0_OBJ) user/user/main.c user/user.ld user/include/bunix/syscall.h user/include/bunix/alloc.h user/include/bunix/id_table.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/user/main.c -o $(BUILD_DIR)/riscv64/user/user.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/user.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_PROC_MODULE): $(RISCV64_USER_CRT0_OBJ) user/proc/main.c user/proc/exec_stack.c user/user.ld user/include/bunix/syscall.h user/include/bunix/alloc.h user/include/bunix/id_table.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/proc/main.c -o $(BUILD_DIR)/riscv64/user/proc.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/proc.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_USER_STRING_OBJ): user/runtime/string.c Makefile
	mkdir -p $(dir $@)
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Wall -Wextra -Werror \
		-c user/runtime/string.c -o $@

$(RISCV64_BLOCK_MODULE): $(RISCV64_USER_CRT0_OBJ) user/block/main.c user/user.ld user/include/bunix/syscall.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/block/main.c -o $(BUILD_DIR)/riscv64/user/block.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/block.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_VFS_MODULE): $(RISCV64_USER_CRT0_OBJ) user/vfs/main.c user/user.ld user/include/bunix/syscall.h user/include/bunix/alloc.h user/include/bunix/id_table.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/vfs/main.c -o $(BUILD_DIR)/riscv64/user/vfs.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/vfs.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_TMPFS_MODULE): $(RISCV64_USER_CRT0_OBJ) user/tmpfs/main.c user/user.ld user/include/bunix/syscall.h user/include/bunix/alloc.h user/include/bunix/id_table.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/tmpfs/main.c -o $(BUILD_DIR)/riscv64/user/tmpfs.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/tmpfs.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_SQUASHFS_MODULE): $(RISCV64_USER_CRT0_OBJ) user/squashfs/main.c user/user.ld user/include/bunix/syscall.h user/include/bunix/alloc.h user/include/bunix/id_table.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/squashfs/main.c -o $(BUILD_DIR)/riscv64/user/squashfs.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/squashfs.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

$(RISCV64_UNIONFS_MODULE): $(RISCV64_USER_CRT0_OBJ) user/unionfs/main.c user/user.ld user/include/bunix/syscall.h user/include/bunix/alloc.h user/include/bunix/id_table.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/unionfs/main.c -o $(BUILD_DIR)/riscv64/user/unionfs.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/unionfs.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)
	$(RISCV64_READELF) -h $@ | grep -F "RISC-V" >/dev/null

test-riscv64-fs-server-build: $(RISCV64_BLOCK_MODULE) $(RISCV64_VFS_MODULE) $(RISCV64_TMPFS_MODULE) $(RISCV64_SQUASHFS_MODULE) $(RISCV64_UNIONFS_MODULE)

$(RISCV64_BOOTSTRAP_MODULE): $(RISCV64_USER_CRT0_OBJ) user/riscv64-bootstrap/main.c user/user.ld user/include/bunix/syscall.h $(RISCV64_USER_RUNTIME_OBJS) Makefile
	mkdir -p $(dir $@) $(BUILD_DIR)/riscv64/user/
	$(RISCV64_CC) $(RISCV64_CC_TARGET_FLAGS) -march=rv64gc -mabi=lp64 -mcmodel=medany \
		-std=c11 -O2 -g -ffreestanding -fno-stack-protector \
		-fno-pic -fno-pie -fno-builtin -Iuser/include -Wall -Wextra -Werror \
		-c user/riscv64-bootstrap/main.c -o $(BUILD_DIR)/riscv64/user/riscv64-bootstrap.c.o
	$(RISCV64_LD) -m elf64lriscv -nostdlib -T user/user.ld -o $@ \
		$(RISCV64_USER_CRT0_OBJ) \
		$(BUILD_DIR)/riscv64/user/riscv64-bootstrap.c.o \
		$(RISCV64_USER_RUNTIME_OBJS)

$(RISCV64_BOOTPKG): $(RISCV64_NAMES_MODULE) $(RISCV64_TIME_MODULE) $(RISCV64_USER_MODULE) $(RISCV64_PROC_MODULE) $(RISCV64_BLOCK_MODULE) $(RISCV64_VFS_MODULE) $(RISCV64_SQUASHFS_MODULE) $(RISCV64_BOOTSTRAP_MODULE) $(RISCV64_USER_ABI_MODULE) $(RISCV64_SLEEP_SMOKE_MODULE) $(RISCV64_SHARED_LINUX_SERVER_MODULE) $(RISCV64_SMOKE_SQUASHFS_IMAGE) tools/build-riscv64-bootpkg.sh
	sh tools/build-riscv64-bootpkg.sh $@ --cmdline "$(RISCV64_KERNEL_CMDLINE)" \
		$(RISCV64_BOOTPKG_SMOKE_ARGS)

$(RISCV64_BOOTPKG_MULTI): $(RISCV64_NAMES_MODULE) $(RISCV64_TIME_MODULE) $(RISCV64_USER_MODULE) $(RISCV64_PROC_MODULE) $(RISCV64_BLOCK_MODULE) $(RISCV64_VFS_MODULE) $(RISCV64_SQUASHFS_MODULE) $(RISCV64_BOOTSTRAP_MODULE) $(RISCV64_USER_ABI_MODULE) $(RISCV64_SLEEP_SMOKE_MODULE) $(RISCV64_SHARED_LINUX_SERVER_MODULE) $(RISCV64_SMOKE_SQUASHFS_IMAGE) tools/build-riscv64-bootpkg.sh
	sh tools/build-riscv64-bootpkg.sh $@ --cmdline "$(RISCV64_KERNEL_CMDLINE)" \
		$(RISCV64_BOOTPKG_SMOKE_ARGS)

$(RISCV64_SLEEP_BOOTPKG): $(RISCV64_NAMES_MODULE) $(RISCV64_TIME_MODULE) $(RISCV64_USER_MODULE) $(RISCV64_PROC_MODULE) $(RISCV64_BLOCK_MODULE) $(RISCV64_VFS_MODULE) $(RISCV64_SQUASHFS_MODULE) $(RISCV64_BOOTSTRAP_MODULE) $(RISCV64_USER_ABI_MODULE) $(RISCV64_SLEEP_SMOKE_MODULE) $(RISCV64_SHARED_LINUX_SERVER_MODULE) $(RISCV64_SMOKE_SQUASHFS_IMAGE) tools/build-riscv64-bootpkg.sh
	sh tools/build-riscv64-bootpkg.sh $@ --cmdline "$(RISCV64_SLEEP_KERNEL_CMDLINE)" \
		$(RISCV64_BOOTPKG_SMOKE_ARGS)

$(RISCV64_ALPINE_BOOTPKG): $(RISCV64_NAMES_MODULE) $(RISCV64_TIME_MODULE) $(RISCV64_USER_MODULE) $(RISCV64_PROC_MODULE) $(RISCV64_BLOCK_MODULE) $(RISCV64_VFS_MODULE) $(RISCV64_SQUASHFS_MODULE) $(RISCV64_BOOTSTRAP_MODULE) $(RISCV64_USER_ABI_MODULE) $(RISCV64_SLEEP_SMOKE_MODULE) $(RISCV64_SHARED_LINUX_SERVER_MODULE) $(RISCV64_ALPINE_SQUASHFS_IMAGE) tools/build-riscv64-bootpkg.sh
	sh tools/build-riscv64-bootpkg.sh $@ --cmdline "$(RISCV64_ALPINE_KERNEL_CMDLINE)" \
		$(RISCV64_BOOTPKG_ALPINE_ARGS)

$(RISCV64_UART_BOOTPKG): $(RISCV64_NAMES_MODULE) $(RISCV64_BOOTSTRAP_MODULE) $(RISCV64_USER_ABI_MODULE) $(RISCV64_SLEEP_SMOKE_MODULE) tools/build-riscv64-bootpkg.sh
	sh tools/build-riscv64-bootpkg.sh $@ --cmdline "$(RISCV64_UART_KERNEL_CMDLINE)" \
		$(RISCV64_BOOTPKG_UART_ARGS)

test-riscv64-bootpkg: $(RISCV64_BOOTPKG) $(RISCV64_BOOTPKG_MULTI)
	grep -aF "BUNIX-RV64-BOOTPKG" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "cmdline $(RISCV64_KERNEL_CMDLINE)" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module names" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module time" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module user" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module proc" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module block" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module vfs" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module squashfs" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module bootstrap" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module abi-smoke.user" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module sleep-smoke.user" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "module linux" $(RISCV64_BOOTPKG) >/dev/null
	grep -aF "cmdline $(RISCV64_KERNEL_CMDLINE)" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module disk0" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module names" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module time" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module user" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module proc" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module block" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module vfs" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module squashfs" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module bootstrap" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module abi-smoke.user" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module sleep-smoke.user" $(RISCV64_BOOTPKG_MULTI) >/dev/null
	grep -aF "module linux" $(RISCV64_BOOTPKG_MULTI) >/dev/null

define X86_USER_LD_MODULE_template
$$($(1)): $$($(1)_OBJS) user/user.ld Makefile
	mkdir -p $$(dir $$@)
	$$(LD) -m elf_x86_64 -nostdlib -T user/user.ld -o $$@ $$($(1)_OBJS)
endef

$(foreach module,$(X86_USER_LD_MODULES),$(eval $(call X86_USER_LD_MODULE_template,$(module))))

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

$(EXECBIG_MODULE): $(EXECOK_MODULE) Makefile FORCE
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

$(USERMEMTEST_MODULE): user/usermemtest/main.c Makefile
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

$(LDSOPATHTEST_MODULE): user/ldsopathtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(FCNTLLOCKTEST_MODULE): user/fcntllocktest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(OPENRCTEST_MODULE): user/openrctest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(PROCFSPIPETEST_MODULE): user/procfspipetest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(SIGNALTEST_MODULE): user/signaltest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(TTYTEST_MODULE): user/ttytest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(TTYSIGTEST_MODULE): user/ttysigtest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(SYSRACETEST_MODULE): user/sysracetest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(SCHEDSTRESS_MODULE): user/schedstress/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(SCHEDBENCH_MODULE): user/schedbench/main.c user/include/bunix/syscall.h Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g -Iuser/include $< -o $@

$(UPTIMETEST_MODULE): user/uptimetest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(NETTEST_MODULE): user/nettest/main.c Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g $< -o $@

$(PRIVTEST_MODULE): user/privtest/main.c user/include/bunix/syscall.h Makefile
	mkdir -p $(dir $@)
	$(MUSL_CC) -static -no-pie -O2 -g -Iuser/include $< -o $@

$(SYNTHETIC_SQUASHFS_IMAGE): tools/build-synthetic-squashfs-rootfs.sh $(ROOTFS_HELLO) $(ROOTFS_SECRET) $(ROOTFS_NESTED) $(ROOTFS_PASSWD) $(ROOTFS_SHADOW) $(ROOTFS_GROUP) $(ROOTFS_INITTAB) $(ROOTFS_EXECS) $(ROOTFS_SPAWNS) $(ROOTFS_SHEBANGTEST) $(ROOTFS_SHEBANGLOOP_A) $(ROOTFS_SHEBANGLOOP_B) $(ROOTFS_SHEBANGBAD) $(ROOTFS_ROOT_MOUNT_SOAK) $(FIRST_MODULE) $(ALLOCTEST_MODULE) $(IPCSTRESS_MODULE) $(LOGIN_MODULE) $(LXTEST_MODULE) $(GETDENTSTEST_MODULE) $(VFORKSTRESS_MODULE) $(LIFECYCLETEST_MODULE) $(EXECOK_MODULE) $(READBIG_MODULE) $(MMAPBIG_MODULE) $(MMAPHUGE_MODULE) $(EXECBIG_MODULE) $(PHDRSTRESS_MODULE) $(MUSL_HELLO_MODULE) $(DYN_HELLO_MODULE) $(FPUTEST_MODULE) $(IOVTEST_MODULE) $(USERMEMTEST_MODULE) $(FCHMODATTEST_MODULE) $(WAITPGIDTEST_MODULE) $(EXECLONGTEST_MODULE) $(AUXIDTEST_MODULE) $(PATHMAXTEST_MODULE) $(PATHERRTEST_MODULE) $(STATIDTEST_MODULE) $(FCNTLLOCKTEST_MODULE) $(OPENRCTEST_MODULE) $(PROCFSPIPETEST_MODULE) $(SIGNALTEST_MODULE) $(TTYTEST_MODULE) $(TTYSIGTEST_MODULE) $(FAULTTEST_MODULE) $(LOWMEMTEST_MODULE) $(SYSRACETEST_MODULE) $(SCHEDSTRESS_MODULE) $(SCHEDBENCH_MODULE) $(UPTIMETEST_MODULE) $(NETTEST_MODULE) $(NETDHCP_MODULE) $(PRIVTEST_MODULE) $(BUSYBOX) $(MUSL_LDSO)
	BUSYBOX=$(BUSYBOX) MUSL_LDSO=$(MUSL_LDSO) MODULE_DIR=$(BUILD_DIR)/modules sh tools/build-synthetic-squashfs-rootfs.sh $@

$(RISCV64_SMOKE_SQUASHFS_IMAGE): tools/build-riscv64-smoke-rootfs.sh $(RISCV64_DYN_HELLO_MODULE) $(RISCV64_MUSL_LDSO) $(RISCV64_SYSCALL_SMOKE_MODULE) $(RISCV64_USERMEMTEST_MODULE) $(RISCV64_MUSL_HELLO_MODULE)
	RISCV64_DYN_HELLO_MODULE=$(RISCV64_DYN_HELLO_MODULE) RISCV64_MUSL_LDSO=$(RISCV64_MUSL_LDSO) RISCV64_SYSCALL_SMOKE_MODULE=$(RISCV64_SYSCALL_SMOKE_MODULE) RISCV64_USERMEMTEST_MODULE=$(RISCV64_USERMEMTEST_MODULE) RISCV64_MUSL_HELLO_MODULE=$(RISCV64_MUSL_HELLO_MODULE) sh tools/build-riscv64-smoke-rootfs.sh $@

$(ALPINE_SQUASHFS_IMAGE): $(LOGIN_MODULE) $(STATIDTEST_MODULE) $(LDSOPATHTEST_MODULE) tools/build-alpine-rootfs.sh tools/alpine-openrc-runlevels.policy modules/passwd modules/shadow modules/group
	ROOTFS_IMAGE_FORMAT=squashfs LOGIN_MODULE=$(LOGIN_MODULE) STATIDTEST_MODULE=$(STATIDTEST_MODULE) LDSOPATHTEST_MODULE=$(LDSOPATHTEST_MODULE) sh tools/build-alpine-rootfs.sh $@

$(RISCV64_ALPINE_SQUASHFS_IMAGE): tools/build-riscv64-alpine-rootfs.sh tools/build-alpine-rootfs.sh tools/alpine-openrc-runlevels.policy $(RISCV64_DYN_HELLO_MODULE) $(RISCV64_MUSL_LDSO)
	RISCV64_DYN_HELLO_MODULE=$(RISCV64_DYN_HELLO_MODULE) RISCV64_MUSL_LDSO=$(RISCV64_MUSL_LDSO) sh tools/build-riscv64-alpine-rootfs.sh $@

$(ROOTFS_FLAVOR_STAMP): FORCE
	mkdir -p $(dir $@)
	@if ! test -f $@ || ! grep -qx '$(ROOTFS_FLAVOR)' $@; then \
		printf '%s\n' '$(ROOTFS_FLAVOR)' > $@; \
	fi

$(VIRTIO_BLK_TEST_IMAGE): $(BLOCK_IMAGE)
	mkdir -p $(dir $@)
	cp $(BLOCK_IMAGE) $@
	chmod u+w $@

$(USB_STORAGE_TEST_IMAGE):
	mkdir -p $(dir $@)
	truncate -s 1M $@

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

$(EFI_BOOT_APP): $(KERNEL) $(GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(NAMES_TEST_MODULE) $(MGMT_TEST_MODULE) $(TIME_MODULE) $(SCHED_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(INPUT_MODULE) $(USB_HID_KBD_MODULE) $(USB_STORAGE_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
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
		"modules/names-test.server=$(NAMES_TEST_MODULE)" \
		"modules/mgmt-test.server=$(MGMT_TEST_MODULE)" \
		"modules/bootstrap.server=$(BOOTSTRAP_MODULE)" \
		"modules/time.server=$(TIME_MODULE)" \
		"modules/sched.server=$(SCHED_MODULE)" \
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
		"modules/input.server=$(INPUT_MODULE)" \
		"modules/usb-hid-kbd.server=$(USB_HID_KBD_MODULE)" \
		"modules/usb-storage.server=$(USB_STORAGE_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(VIRTIO_BLK_TEST_EFI_BOOT_APP): $(KERNEL) $(VIRTIO_BLK_TEST_GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(SCHED_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(INPUT_MODULE) $(USB_HID_KBD_MODULE) $(USB_STORAGE_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VIRTIO_BLK_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
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
		"modules/sched.server=$(SCHED_MODULE)" \
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
		"modules/input.server=$(INPUT_MODULE)" \
		"modules/usb-hid-kbd.server=$(USB_HID_KBD_MODULE)" \
		"modules/usb-storage.server=$(USB_STORAGE_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/virtio-blk.server=$(VIRTIO_BLK_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(VIRTIO_NET_TEST_EFI_BOOT_APP): $(KERNEL) $(VIRTIO_NET_TEST_GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(SCHED_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(INPUT_MODULE) $(USB_HID_KBD_MODULE) $(USB_STORAGE_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VIRTIO_NET_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
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
		"modules/sched.server=$(SCHED_MODULE)" \
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
		"modules/input.server=$(INPUT_MODULE)" \
		"modules/usb-hid-kbd.server=$(USB_HID_KBD_MODULE)" \
		"modules/usb-storage.server=$(USB_STORAGE_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/virtio-net.server=$(VIRTIO_NET_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(EXT2_TEST_EFI_BOOT_APP): $(KERNEL) $(EXT2_TEST_GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(MGMT_TEST_MODULE) $(TIME_MODULE) $(SCHED_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(EXT2_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(INPUT_MODULE) $(USB_HID_KBD_MODULE) $(USB_STORAGE_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE) $(EXT2_TEST_IMAGE)
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
		"modules/mgmt-test.server=$(MGMT_TEST_MODULE)" \
		"modules/bootstrap.server=$(BOOTSTRAP_MODULE)" \
		"modules/time.server=$(TIME_MODULE)" \
		"modules/sched.server=$(SCHED_MODULE)" \
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
		"modules/input.server=$(INPUT_MODULE)" \
		"modules/usb-hid-kbd.server=$(USB_HID_KBD_MODULE)" \
		"modules/usb-storage.server=$(USB_STORAGE_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/ext2-test.img=$(EXT2_TEST_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(EXT2_FSCK_TEST_EFI_BOOT_APP): $(KERNEL) $(EXT2_FSCK_TEST_GRUB_STANDALONE_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(SCHED_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(EXT2_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(INPUT_MODULE) $(USB_HID_KBD_MODULE) $(USB_STORAGE_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VIRTIO_BLK_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
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
		"modules/sched.server=$(SCHED_MODULE)" \
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
		"modules/input.server=$(INPUT_MODULE)" \
		"modules/usb-hid-kbd.server=$(USB_HID_KBD_MODULE)" \
		"modules/usb-storage.server=$(USB_STORAGE_MODULE)" \
		"modules/virtio-bus.server=$(VIRTIO_BUS_MODULE)" \
		"modules/net.server=$(NET_MODULE)" \
		"modules/netcfg.server=$(NETCFG_MODULE)" \
		"modules/virtio-blk.server=$(VIRTIO_BLK_MODULE)" \
		"modules/vfs.server=$(VFS_MODULE)" \
		"modules/ping.server=$(PING_MODULE)" \
		"modules/disk0.img=$(BLOCK_IMAGE)" \
		"modules/vm.server=modules/vm.server"

$(EFI_BOOT_IMG): $(KERNEL) $(GRUB_CFG) $(ROOTFS_FLAVOR_STAMP) $(BOOTSTRAP_MODULE) $(CONSOLE_MODULE) $(NAMES_MODULE) $(TIME_MODULE) $(SCHED_MODULE) $(USER_MODULE) $(LINUX_SERVER_MODULE) $(PROC_MODULE) $(PROCFS_MODULE) $(TMPFS_MODULE) $(DEVFS_MODULE) $(SYSFS_MODULE) $(UTMPFS_MODULE) $(SQUASHFS_MODULE) $(UNIONFS_MODULE) $(BLOCK_MODULE) $(PCI_MODULE) $(USB_BUS_MODULE) $(USB_SYNTH_MODULE) $(XHCI_MODULE) $(INPUT_MODULE) $(USB_HID_KBD_MODULE) $(USB_STORAGE_MODULE) $(VIRTIO_BUS_MODULE) $(NET_MODULE) $(NETCFG_MODULE) $(VFS_MODULE) $(PING_MODULE) modules/vm.server $(BLOCK_IMAGE)
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
	cp $(SCHED_MODULE) $(ISO_ROOT)/modules/sched.server
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
	cp $(INPUT_MODULE) $(ISO_ROOT)/modules/input.server
	cp $(USB_HID_KBD_MODULE) $(ISO_ROOT)/modules/usb-hid-kbd.server
	cp $(USB_STORAGE_MODULE) $(ISO_ROOT)/modules/usb-storage.server
	cp $(VIRTIO_BUS_MODULE) $(ISO_ROOT)/modules/virtio-bus.server
	cp $(NET_MODULE) $(ISO_ROOT)/modules/net.server
	cp $(NETCFG_MODULE) $(ISO_ROOT)/modules/netcfg.server
	cp $(VFS_MODULE) $(ISO_ROOT)/modules/vfs.server
	cp $(PING_MODULE) $(ISO_ROOT)/modules/ping.server
	cp $(BLOCK_IMAGE) $(ISO_ROOT)/modules/disk0.img
	cp modules/vm.server $(ISO_ROOT)/modules/vm.server
	$(GRUB_MKRESCUE) -o $@ $(ISO_ROOT)

RUN_ROOTFS_FLAVOR ?= squashfs
RUN_QEMU_NET_ARGS ?= $(if $(filter alpine-squashfs,$(RUN_ROOTFS_FLAVOR)),$(QEMU_VIRTIO_NET_EXTERNAL_ARGS),)
RUN_PROFILE_ROOTFS_TARGET ?= $(if $(filter alpine-squashfs,$(RUN_ROOTFS_FLAVOR)),$(ALPINE_SQUASHFS_IMAGE),$(SYNTHETIC_SQUASHFS_IMAGE))
RUN_PROFILE_ESP_DIR ?= $(if $(filter alpine-squashfs,$(RUN_ROOTFS_FLAVOR)),$(VIRTIO_NET_TEST_ESP_DIR),$(ESP_DIR))
RUN_PROFILE_BUILD_TARGET ?= $(if $(filter alpine-squashfs,$(RUN_ROOTFS_FLAVOR)),$(VIRTIO_NET_TEST_EFI_BOOT_APP),$(EFI_BOOT_APP))
RUN_PROFILE_NETWORK_CHECK ?= auto

run: run-alpine-net

run-fast:
	$(MAKE) ROOTFS_FLAVOR=squashfs RUN_QEMU_NET_ARGS= run-fast-qemu

run-fast-qemu: $(EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ESP_DIR) \
		-serial stdio -display none -no-reboot

run-profile:
	RUN_PROFILE_ROOTFS_FLAVOR=$(RUN_ROOTFS_FLAVOR) \
		RUN_PROFILE_ROOTFS_TARGET=$(RUN_PROFILE_ROOTFS_TARGET) \
		RUN_PROFILE_ESP_DIR=$(RUN_PROFILE_ESP_DIR) \
		RUN_PROFILE_BUILD_TARGET=$(RUN_PROFILE_BUILD_TARGET) \
		RUN_PROFILE_NETWORK_CHECK=$(RUN_PROFILE_NETWORK_CHECK) \
		RUN_QEMU_NET_ARGS="$(RUN_QEMU_NET_ARGS)" \
		OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		sh tools/run-profile.sh

run-profile-fast:
	$(MAKE) RUN_ROOTFS_FLAVOR=squashfs RUN_QEMU_NET_ARGS= RUN_PROFILE_NETWORK_CHECK=skip run-profile

run-profile-alpine:
	$(MAKE) RUN_ROOTFS_FLAVOR=alpine-squashfs RUN_QEMU_NET_ARGS= RUN_PROFILE_ESP_DIR=$(ALPINE_ESP_DIR) RUN_PROFILE_BUILD_TARGET=$(ALPINE_EFI_BOOT_APP) RUN_PROFILE_NETWORK_CHECK=skip run-profile

run-profile-alpine-net:
	$(MAKE) RUN_ROOTFS_FLAVOR=alpine-squashfs RUN_QEMU_NET_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" RUN_PROFILE_NETWORK_CHECK=required run-profile

test-run-profile-log-volume: run-profile tools/check-run-profile-log-volume.sh
	latest=$$(ls -td build/run-profile/* | head -n 1); \
		sh tools/check-run-profile-log-volume.sh "$$latest"

test-run-fast:
	$(MAKE) ROOTFS_FLAVOR=squashfs $(EFI_BOOT_APP)
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=60s BUNIX_USER=root BUNIX_PASSWORD=root \
		BUNIX_PROMPT='~ # ' BUNIX_MARKER=BUNIX_RUN_FAST_OK \
		BUNIX_CMD='echo fast-profile; test -x /bin/sh' \
		QEMU_EXTRA_ARGS= sh tools/test-command.sh

test-run-alpine:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs ESP_DIR=$(ALPINE_ESP_DIR) $(ALPINE_EFI_BOOT_APP)
	ESP_DIR=$(ALPINE_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=180s BUNIX_USER=root BUNIX_PASSWORD=root \
		BUNIX_PROMPT='~ # ' BUNIX_MARKER=BUNIX_RUN_ALPINE_OK \
		BUNIX_CMD='test -e /run/openrc/softlevel; cat /proc/boot_timing >/dev/null' \
		QEMU_EXTRA_ARGS= sh tools/test-command.sh

test-run-alpine-net: test-boot-virtio-net-networking

run-alpine:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs RUN_QEMU_NET_ARGS= run-alpine-qemu

run-alpine-qemu: $(ALPINE_EFI_BOOT_APP)
	$(QEMU) -enable-kvm -machine q35 -cpu host -m 128M \
		-smp $(SMP) \
		-drive if=pflash,format=raw,readonly=on,file=$(OVMF_CODE) \
		-drive format=raw,file=fat:rw:$(ALPINE_ESP_DIR) \
		-serial stdio -display none -no-reboot

run-alpine-net:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs RUN_QEMU_NET_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" run-alpine-net-qemu

run-alpine-net-qemu: $(VIRTIO_NET_TEST_EFI_BOOT_APP)
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

run-riscv64-alpine: $(RISCV64_ALPINE_BOOTPKG)
	$(MAKE) ARCH=riscv64 all
	$(RISCV64_QEMU) -machine virt -m 128M -nographic -no-reboot \
		-kernel $(RISCV64_KERNEL) -initrd $(RISCV64_ALPINE_BOOTPKG)

test: test-parallel

test-alpine-rootfs: $(LOGIN_MODULE) $(STATIDTEST_MODULE) $(LDSOPATHTEST_MODULE) tools/build-alpine-rootfs.sh tools/alpine-openrc-runlevels.policy tools/test-alpine-rootfs.sh modules/passwd modules/shadow modules/group
	LOGIN_MODULE=$(LOGIN_MODULE) STATIDTEST_MODULE=$(STATIDTEST_MODULE) LDSOPATHTEST_MODULE=$(LDSOPATHTEST_MODULE) sh tools/test-alpine-rootfs.sh

test-alpine-rootfs-stock-networking: test-alpine-rootfs

test-riscv64-dynamic-linker-artifacts: $(RISCV64_DYN_HELLO_MODULE) $(RISCV64_MUSL_LDSO)
	test -s $(RISCV64_DYN_HELLO_MODULE)
	test -s $(RISCV64_MUSL_LDSO)
	$(RISCV64_READELF) -l $(RISCV64_DYN_HELLO_MODULE) | grep -F "/lib/ld-musl-riscv64.so.1" >/dev/null

$(BUILD_DIR)/test-linux-exec-abi: tools/test-linux-exec-abi.c user/linux/exec_abi.c Makefile
	mkdir -p $(dir $@)
	$(HOSTCC) -std=c11 -O2 -g -Wall -Wextra -Werror $< -o $@

test-linux-exec-abi: $(BUILD_DIR)/test-linux-exec-abi
	$(BUILD_DIR)/test-linux-exec-abi

$(BUILD_DIR)/test-proc-exec-stack: tools/test-proc-exec-stack.c user/proc/exec_stack.c user/include/bunix/syscall.h Makefile
	mkdir -p $(dir $@)
	$(HOSTCC) -std=c11 -O2 -g -Wall -Wextra -Werror -Iuser/include $< -o $@

test-proc-exec-stack: $(BUILD_DIR)/test-proc-exec-stack
	$(BUILD_DIR)/test-proc-exec-stack

test-riscv64-alpine-rootfs: tools/build-riscv64-alpine-rootfs.sh tools/build-alpine-rootfs.sh tools/test-riscv64-alpine-rootfs.sh tools/alpine-openrc-runlevels.policy $(RISCV64_DYN_HELLO_MODULE) $(RISCV64_MUSL_LDSO)
	RISCV64_DYN_HELLO_MODULE=$(RISCV64_DYN_HELLO_MODULE) RISCV64_MUSL_LDSO=$(RISCV64_MUSL_LDSO) sh tools/test-riscv64-alpine-rootfs.sh

test-path-safety: tools/path-safety.sh tools/test-path-safety.sh
	sh tools/test-path-safety.sh

test-riscv64-shared-linux-server-build: $(RISCV64_SHARED_LINUX_SERVER_MODULE)
	test -s $(RISCV64_SHARED_LINUX_SERVER_MODULE)

test-riscv64-proc-server-build: $(RISCV64_PROC_MODULE)
	test -s $(RISCV64_PROC_MODULE)

test-boot: $(EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-squashfs.txt tools/test-boot-markers-squashfs-up.txt tools/test-boot-markers-alpine-smoke.txt tools/test-boot-markers-alpine-squashfs.txt
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) QEMU_TIMEOUT=$(QEMU_TIMEOUT) \
		BUNIX_BOOT_PHASE="$(BUNIX_BOOT_PHASE)" BUNIX_BOOT_MARKER="$(BUNIX_BOOT_MARKER)" \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log $(TEST_BOOT_MARKERS)

test-boot-alpine-stock-networking: $(ALPINE_SQUASHFS_IMAGE) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-alpine-smoke.txt
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs VIRTIO_NET_TEST_ESP_DIR=$(BUILD_DIR)/esp-virtio-net-alpine-stock-networking $(BUILD_DIR)/esp-virtio-net-alpine-stock-networking/EFI/BOOT/BOOTX64.EFI
	ESP_DIR=$(BUILD_DIR)/esp-virtio-net-alpine-stock-networking OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) QEMU_TIMEOUT=$(if $(filter command\ line environment,$(origin QEMU_TIMEOUT)),$(QEMU_TIMEOUT),180s) \
		ROOTFS_FLAVOR=alpine-squashfs SERIAL_LOG=$(BUILD_DIR)/serial.log QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-alpine-smoke.txt

test-boot-openrc-fork-reclaim:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs KERNEL_CMDLINE="log=info" test-boot-openrc-fork-reclaim-run

test-boot-openrc-fork-reclaim-run: $(EFI_BOOT_APP) tools/guest-openrc-fork-reclaim.sh tools/test-lib.sh tools/test-command.sh
	ROOTFS_FLAVOR=alpine-squashfs BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_LOGIN_TIMEOUT=180 BUNIX_COMMAND_TIMEOUT=180 \
		BUNIX_MARKER=BUNIX_OPENRC_FORK_RECLAIM_OK \
		BUNIX_CMD_FILE=tools/guest-openrc-fork-reclaim.sh \
		sh tools/test-command.sh

test-boot-net-route:
	$(MAKE) TEST_BOOT_MARKERS=tools/test-boot-markers-net-route.txt test-boot

test-boot-handle-race:
	$(MAKE) KERNEL_CMDLINE="log=info handle-race-selftest" TEST_BOOT_MARKERS=tools/test-boot-markers-handle-race.txt test-boot

test-linux-lifecycle: $(EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_LINUX_LIFECYCLE_OK \
		BUNIX_CMD='/bin/lifecycletest && echo BUNIX_LINUX_LIFECYCLE_OK' \
		sh tools/test-command.sh

test-names-authority:
	$(MAKE) KERNEL_CMDLINE="log=info names-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="names-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-names-authority.txt test-boot

test-management-authority:
	$(MAKE) KERNEL_CMDLINE="log=info mgmt-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="mgmt-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-management-authority.txt test-boot

test-tty-input-authority:
	$(MAKE) KERNEL_CMDLINE="log=info tty-input-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="tty-input-auth-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-tty-input-authority.txt test-boot

test-vfs-authority:
	$(MAKE) KERNEL_CMDLINE="log=info vfs-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="vfs-auth-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-vfs-authority.txt test-boot

test-vfs-mount-authority:
	$(MAKE) KERNEL_CMDLINE="log=info vfs-mount-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="vfs-mount-auth-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-vfs-mount-authority.txt test-boot

test-vfs-subject-authority:
	$(MAKE) KERNEL_CMDLINE="log=info vfs-subject-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="vfs-subject-auth-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-vfs-subject-authority.txt test-boot

test-tmpfs-subject-authority:
	$(MAKE) KERNEL_CMDLINE="log=info tmpfs-subject-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="tmpfs-subject-auth-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-tmpfs-subject-authority.txt test-boot

test-squashfs-subject-authority:
	$(MAKE) KERNEL_CMDLINE="log=info squashfs-subject-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="squashfs-subject-auth-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-squashfs-subject-authority.txt test-boot

test-procfs-subject-authority:
	$(MAKE) KERNEL_CMDLINE="log=info procfs-subject-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="procfs-subject-auth-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-procfs-subject-authority.txt test-boot

test-unionfs-subject-authority:
	$(MAKE) KERNEL_CMDLINE="log=info unionfs-subject-auth-test" BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="unionfs-subject-auth-test: ok" TEST_BOOT_MARKERS=tools/test-boot-markers-unionfs-subject-authority.txt test-boot

test-lowmem-isolation: $(EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_CMD='/bin/lowmemtest' sh tools/test-command.sh

test-user-memory-contract: $(EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_CMD='/bin/usermemtest' sh tools/test-command.sh

test-boot-riscv64-early: $(RISCV64_BOOTPKG) tools/test-riscv64-boot.sh tools/check-markers.sh tools/test-riscv64-markers-early.txt
	$(MAKE) ARCH=riscv64 all
	RISCV64_QEMU=$(RISCV64_QEMU) RISCV64_KERNEL=$(RISCV64_KERNEL) \
		RISCV64_INITRD=$(RISCV64_BOOTPKG) \
		RISCV64_SERIAL_LOG=$(RISCV64_EARLY_SERIAL_LOG) \
		RISCV64_QEMU_TIMEOUT=30s \
		RISCV64_MARKERS=tools/test-riscv64-markers-early.txt \
		sh tools/test-riscv64-boot.sh

test-boot-riscv64-sleep: $(RISCV64_SLEEP_BOOTPKG) tools/test-riscv64-boot.sh tools/check-markers.sh tools/test-riscv64-markers-sleep.txt
	$(MAKE) ARCH=riscv64 all
	RISCV64_QEMU=$(RISCV64_QEMU) RISCV64_KERNEL=$(RISCV64_KERNEL) \
		RISCV64_INITRD=$(RISCV64_SLEEP_BOOTPKG) \
		RISCV64_SERIAL_LOG=$(BUILD_DIR)/riscv64/test-boot-sleep.serial.log \
		RISCV64_QEMU_TIMEOUT=30s \
		RISCV64_MARKERS=tools/test-riscv64-markers-sleep.txt \
		sh tools/test-riscv64-boot.sh

test-boot-riscv64-alpine: $(RISCV64_ALPINE_BOOTPKG) tools/test-riscv64-boot.sh tools/check-markers.sh tools/test-riscv64-markers-alpine.txt
	$(MAKE) ARCH=riscv64 all
	RISCV64_QEMU=$(RISCV64_QEMU) RISCV64_KERNEL=$(RISCV64_KERNEL) \
		RISCV64_INITRD=$(RISCV64_ALPINE_BOOTPKG) \
		RISCV64_SERIAL_LOG=$(RISCV64_ALPINE_SERIAL_LOG) \
		RISCV64_QEMU_TIMEOUT=45s \
		RISCV64_MARKERS=tools/test-riscv64-markers-alpine.txt \
		sh tools/test-riscv64-boot.sh

test-boot-riscv64-uart-console: $(RISCV64_UART_BOOTPKG) tools/test-riscv64-boot.sh tools/check-markers.sh tools/test-riscv64-markers-uart-console.txt
	$(MAKE) ARCH=riscv64 all
	RISCV64_QEMU=$(RISCV64_QEMU) RISCV64_KERNEL=$(RISCV64_KERNEL) \
		RISCV64_INITRD=$(RISCV64_UART_BOOTPKG) \
		RISCV64_SERIAL_LOG=$(RISCV64_UART_SERIAL_LOG) \
		RISCV64_QEMU_TIMEOUT=30s \
		RISCV64_MARKERS=tools/test-riscv64-markers-uart-console.txt \
		sh tools/test-riscv64-boot.sh

test-riscv64-log-isolation:
	test "$(RISCV64_EARLY_SERIAL_LOG_DEFAULT)" != "$(RISCV64_ALPINE_SERIAL_LOG_DEFAULT)"
	test "$(RISCV64_EARLY_SERIAL_LOG_DEFAULT)" != "$(RISCV64_UART_SERIAL_LOG_DEFAULT)"
	test "$(RISCV64_ALPINE_SERIAL_LOG_DEFAULT)" != "$(RISCV64_UART_SERIAL_LOG_DEFAULT)"
	printf '%s\n' "$(RISCV64_EARLY_SERIAL_LOG_DEFAULT)" "$(RISCV64_ALPINE_SERIAL_LOG_DEFAULT)" "$(RISCV64_UART_SERIAL_LOG_DEFAULT)"

test-boot-ext2: $(EXT2_TEST_EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-ext2.txt
	ESP_DIR=$(EXT2_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-ext2.txt

test-ext2-subject-authority: EXT2_TEST_CMDLINE=log=info ext2-subject-auth-test
test-ext2-subject-authority: $(EXT2_TEST_EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-ext2-subject-authority.txt
	ESP_DIR=$(EXT2_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="ext2-subject-auth-test: ok" \
		sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-ext2-subject-authority.txt

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

test-boot-usb-hid-kbd: KERNEL_CMDLINE=log=info xhci-test xhci-hid-test
test-boot-usb-hid-kbd: $(EFI_BOOT_APP) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-usb-hid-kbd.txt tools/qmp-send-key.py
	BUNIX_TEST_RUNTIME_DIR=/tmp/bunix-usb-hid-kbd-test \
		ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="machine: poweroff" \
		BUNIX_TEST_SIDECAR_CMD="python3 tools/qmp-send-key.py /tmp/bunix-usb-hid-kbd-test/qmp.sock $(BUILD_DIR)/serial.log" \
		QEMU_EXTRA_ARGS="$(QEMU_XHCI_ARGS) -qmp unix:/tmp/bunix-usb-hid-kbd-test/qmp.sock,server=on,wait=off" \
		sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-usb-hid-kbd.txt

test-boot-usb-storage: KERNEL_CMDLINE=log=info xhci-test xhci-storage-test
test-boot-usb-storage: $(EFI_BOOT_APP) $(USB_STORAGE_TEST_IMAGE) tools/check-markers.sh tools/test-lib.sh tools/test-boot.sh tools/test-boot-markers-usb-storage.txt
	ESP_DIR=$(ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		ROOTFS_FLAVOR=$(ROOTFS_FLAVOR) SERIAL_LOG=$(BUILD_DIR)/serial.log \
		BUNIX_BOOT_PHASE=marker-poweroff BUNIX_BOOT_MARKER="machine: poweroff" \
		QEMU_EXTRA_ARGS="$(QEMU_XHCI_STORAGE_ARGS)" sh tools/test-boot.sh
	sh tools/check-markers.sh $(BUILD_DIR)/serial.log tools/test-boot-markers-usb-storage.txt

test-boot-usb:
	$(MAKE) test-boot-usb-synth
	$(MAKE) test-boot-xhci-discovery
	$(MAKE) test-boot-usb-hid-kbd

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
	! grep -aF "netcfg: dhcp fallback" $(BUILD_DIR)/serial.log >/dev/null

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
		BUNIX_CMD='C=/proc/net/config; test -e /run/openrc/started/localmount; rc-service localmount status; test -e /run/openrc/started/networking; rc-service networking status; cat $$C; grep -F "iface eth0" $$C; grep -F "default_ipv4 1" $$C; grep -F "rxq 0 txq 0" $$C; ping -c1 -W4 10.0.2.2; ping -c1 -W4 4.2.2.1; ping -c1 -W4 8.8.8.8' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-external-stack:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-external-ping-strict-run
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-dns-wget-run

test-boot-virtio-net-external-ping-strict:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-external-ping-strict-run

test-boot-virtio-net-external-ping-strict-run: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=300s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_LOGIN_TIMEOUT=180 \
		BUNIX_COMMAND_TIMEOUT=240 \
		BUNIX_FAILURE_GUEST_PROBES=0 \
		BUNIX_MARKER=BUNIX_NET_EXT_PING_OK \
		BUNIX_CMD='test -e /run/openrc/started/networking; rc-service networking status; busybox ping -c 4 -W 4 4.2.2.1 | busybox tee /tmp/p; busybox grep -F "4 packets transmitted, 4 packets received" /tmp/p; test "$$(busybox grep -c "64 bytes from 4.2.2.1:.*ttl=255" /tmp/p)" = 4' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-dns-wget:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-dns-wget-run

test-boot-virtio-net-dns-wget-run: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/guest-network-dns-wget.sh tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=360s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_LOGIN_TIMEOUT=180 \
		BUNIX_COMMAND_TIMEOUT=300 \
		BUNIX_SEND_DELAY=0.2 \
		BUNIX_FAILURE_GUEST_PROBES=0 \
		BUNIX_MARKER=BUNIX_NET_DNS_WGET_OK \
		BUNIX_CMD_FILE=tools/guest-network-dns-wget.sh \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-apk-add:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-apk-add-run

test-boot-virtio-net-apk-add-run: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/guest-apk-install-smoke.sh tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=420s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_LOGIN_TIMEOUT=180 \
		BUNIX_COMMAND_TIMEOUT=360 \
		BUNIX_SEND_DELAY=0.2 \
		BUNIX_FAILURE_GUEST_PROBES=0 \
		BUNIX_MARKER=BUNIX_APK_INSTALL_OK \
		BUNIX_CMD_FILE=tools/guest-apk-install-smoke.sh \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-wget-readv:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-wget-readv-run

test-boot-virtio-net-wget-readv-run: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/guest-network-wget-readv.sh tools/http-readv-fixture.py tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=240s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_LOGIN_TIMEOUT=180 \
		BUNIX_COMMAND_TIMEOUT=180 \
		BUNIX_SEND_DELAY=0.2 \
		BUNIX_FAILURE_GUEST_PROBES=0 \
		BUNIX_MARKER=BUNIX_NET_WGET_READV_OK \
		BUNIX_CMD_FILE=tools/guest-network-wget-readv.sh \
		BUNIX_TEST_SIDECAR_READY_FILE='$(BUILD_DIR)/http-readv-fixture.ready' \
		BUNIX_TEST_SIDECAR_CMD='rm -f $(BUILD_DIR)/http-readv-fixture.ready; python3 tools/http-readv-fixture.py --host 0.0.0.0 --port 18080 --ready-file $(BUILD_DIR)/http-readv-fixture.ready --duration 240' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_EXTERNAL_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-socket-peer: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/virtio-net-peer.py tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=120s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_SOCKET_PEER_OK \
		BUNIX_TEST_SIDECAR_READY_FILE='$(BUILD_DIR)/virtio-net-peer.ready' \
		BUNIX_TEST_SIDECAR_CMD='rm -f $(BUILD_DIR)/virtio-net-peer.ready; python3 tools/virtio-net-peer.py --mcast $(QEMU_VIRTIO_NET_SOCKET_MCAST) --ready-file $(BUILD_DIR)/virtio-net-peer.ready --duration 120 --verbose' \
		BUNIX_CMD='cat /proc/net/config; busybox grep -F "iface eth0" /proc/net/config; busybox grep -F "default_ipv4 1" /proc/net/config; busybox ping -c 1 -W 4 10.0.2.2; cat /proc/net/dev; set -- $$(busybox grep -F "eth0:" /proc/net/dev); test "$$2" != 0; test "$$10" != 0' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_SOCKET_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-socket-peer-ipv6: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/virtio-net-peer.py tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=120s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_SOCKET_PEER_IPV6_OK \
		BUNIX_TEST_SIDECAR_READY_FILE='$(BUILD_DIR)/virtio-net-peer.ready' \
		BUNIX_TEST_SIDECAR_CMD='rm -f $(BUILD_DIR)/virtio-net-peer.ready; python3 tools/virtio-net-peer.py --mcast $(QEMU_VIRTIO_NET_SOCKET_MCAST) --ready-file $(BUILD_DIR)/virtio-net-peer.ready --duration 120 --verbose' \
		BUNIX_CMD='cat /proc/net/config; busybox grep -F "iface eth0" /proc/net/config; busybox ping -6 -c 1 -W 4 2001:db8:18::2; cat /proc/net/ndisc; busybox grep -F "2001:0DB8:0018:0000:0000:0000:0000:0002" /proc/net/ndisc; cat /proc/net/dev; set -- $$(busybox grep -F "eth0:" /proc/net/dev); test "$$2" != 0; test "$$10" != 0' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_SOCKET_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-socket-peer-udp6: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/virtio-net-peer.py tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=120s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_SOCKET_PEER_UDP6_OK \
		BUNIX_TEST_SIDECAR_READY_FILE='$(BUILD_DIR)/virtio-net-peer.ready' \
		BUNIX_TEST_SIDECAR_CMD='rm -f $(BUILD_DIR)/virtio-net-peer.ready; python3 tools/virtio-net-peer.py --mcast $(QEMU_VIRTIO_NET_SOCKET_MCAST) --ready-file $(BUILD_DIR)/virtio-net-peer.ready --duration 120 --verbose' \
		BUNIX_CMD='/bin/nettest udp6-external; cat /proc/net/ndisc; busybox grep -F "2001:0DB8:0018:0000:0000:0000:0000:0002" /proc/net/ndisc; cat /proc/net/dev; set -- $$(busybox grep -F "eth0:" /proc/net/dev); test "$$2" != 0; test "$$10" != 0' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_SOCKET_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-socket-peer-tcp6: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/virtio-net-peer.py tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=120s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_SOCKET_PEER_TCP6_OK \
		BUNIX_TEST_SIDECAR_READY_FILE='$(BUILD_DIR)/virtio-net-peer.ready' \
		BUNIX_TEST_SIDECAR_CMD='rm -f $(BUILD_DIR)/virtio-net-peer.ready; python3 tools/virtio-net-peer.py --mcast $(QEMU_VIRTIO_NET_SOCKET_MCAST) --ready-file $(BUILD_DIR)/virtio-net-peer.ready --duration 120 --verbose --tcp6-syn-port 23459' \
		BUNIX_CMD='/bin/nettest tcp6-external-connect; /bin/nettest tcp6-external-accept; cat /proc/net/ndisc; busybox grep -F "2001:0DB8:0018:0000:0000:0000:0000:0002" /proc/net/ndisc; cat /proc/net/dev; set -- $$(busybox grep -F "eth0:" /proc/net/dev); test "$$2" != 0; test "$$10" != 0' \
		QEMU_EXTRA_ARGS="$(QEMU_VIRTIO_NET_SOCKET_ARGS)" sh tools/test-command.sh

test-boot-virtio-net-external-ping:
	$(MAKE) ROOTFS_FLAVOR=alpine-squashfs test-boot-virtio-net-external-ping-run

test-boot-virtio-net-external-ping-run: $(VIRTIO_NET_TEST_EFI_BOOT_APP) tools/test-lib.sh tools/test-command.sh
	ESP_DIR=$(VIRTIO_NET_TEST_ESP_DIR) OVMF_CODE=$(OVMF_CODE) QEMU=$(QEMU) SMP=$(SMP) \
		QEMU_TIMEOUT=180s BUNIX_USER=root BUNIX_PASSWORD=root BUNIX_PROMPT='~ # ' \
		BUNIX_MARKER=BUNIX_EXTERNAL_PING_OK \
		BUNIX_CMD='test -e /run/openrc/started/networking; rc-service networking status; C=/proc/net/config; grep -F "iface eth0" $$C; grep -F "default_ipv4 1" $$C; ping -c4 -W4 4.2.2.1 >/tmp/p; cat /tmp/p; grep -F "64 bytes from 4.2.2.1:" /tmp/p; grep -F "4 packets transmitted, 4 packets received" /tmp/p; ping -c1 -W4 8.8.8.8' \
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

test-shell test-shell-part test-smoke test-smoke-parallel test-shell-parallel: KERNEL_CMDLINE=log=info shell-test

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
