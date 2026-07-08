# riscv64 BPI-F3 bringup notes

This document is the hardware bringup plan for Banana Pi BPI-F3.  It starts
after the QEMU `virt` riscv64 userspace path is working, so board debugging can
focus on firmware, FDT, console, timer, interrupt, and shutdown behavior.

## Reference set

The first bringup reference set is documentation-only unless the referenced
code is license-compatible with Bunix:

- Banana Pi BPI-F3 docs: <https://docs.banana-pi.org/en/BPI-F3/BananaPi_BPI-F3>
- Bianbu Linux 6.1 source: <https://gitee.com/bianbu-linux/linux-6.1>.
  Anonymous `git ls-remote` did not resolve this mirror in the July 8, 2026
  bringup environment, so the BPI-SINOVOIP Linux branch below is the pinned
  Linux/DT reference for now.
- Bianbu U-Boot 2022.10 source:
  <https://gitee.com/bianbu-linux/uboot-2022.10>,
  `HEAD=46a4f510352684407c074b7c0e9114b5443dcc59`.
- Bianbu OpenSBI source: <https://gitee.com/bianbu-linux/opensbi>,
  `HEAD=34143f5f665be8b86ebb71b8085a1ade7f8b97ad`.
- BPI-SINOVOIP OpenSBI branch:
  <https://github.com/BPI-SINOVOIP/pi-opensbi/tree/v1.3-k1>,
  `v1.3-k1=adc95bdae2fa48250dada0501cb272163bc14432`.
- BPI-SINOVOIP U-Boot branch:
  <https://github.com/BPI-SINOVOIP/pi-u-boot/tree/v2022.10-k1>,
  `v2022.10-k1=cdddb8e05f3a348bd2e847e85ba622f29c966a88`.
- BPI-SINOVOIP Linux branch:
  <https://github.com/BPI-SINOVOIP/pi-linux/tree/linux-6.1.15-k1>,
  `linux-6.1.15-k1=eb4e73284582351b02517a157f39d8366ccb57c5`.
- SpacemiT RISC-V IME extension spec: <https://github.com/space-mit/riscv-ime-extension-spec>
- BPI-F3 schematic link from Banana Pi docs:
  <https://drive.google.com/file/d/19iLJ5xnCB_oK8VeQjkPGjzAn39WYyylv/view?usp=sharing>
- U-Boot `bootelf` command documentation:
  <https://docs.u-boot.org/en/latest/usage/cmd/bootelf.html>

Reference facts captured from the Banana Pi docs:

- Board: Banana Pi BPI-F3 industrial RISC-V development board.
- SoC: SpacemiT K1, 8-core RISC-V CPU.
- CPU ISA description: X60 `RV64GCVB`, RVA22, RVV 1.0.
- Memory: 2/4/8/16 GB LPDDR4.
- Storage: 8/16/32/128 GB eMMC options plus microSD.
- I/O relevant to later bringup: two GbE ports, USB 3.0 host, USB-C OTG,
  PCIe/M.2, HDMI, GPIO, and multiple UARTs.
- Published firmware/BSP references include OpenSBI, U-Boot 2022.10, Linux
  6.1, schematic, and SpacemiT K1 documentation links.

## Boot chain

The intended first Bunix hardware boot chain is:

1. Board ROM selects the storage source according to the BPI-F3 boot media
   configuration.
2. Vendor firmware initializes DRAM and enters OpenSBI.
3. OpenSBI enters vendor U-Boot in supervisor mode.
4. The operator stops at the U-Boot prompt and loads Bunix artifacts from a
   boot partition.
5. U-Boot loads `bunixos-riscv64.elf`, `bunix-riscv64.bootpkg`, and uses the
   firmware-provided FDT.
6. U-Boot updates `/chosen` with `bootargs`, `linux,initrd-start`, and
   `linux,initrd-end`, then enters Bunix with `bootelf -d`.
7. Bunix uses the same FDT memory/initrd discovery path proven in QEMU.

This deliberately avoids replacing board firmware for the first milestone.
The first board proof should use firmware-provided FDT and SBI services before
adding native K1 UART, interrupt-controller, storage, or reset drivers.

## Artifact

The first Bunix BPI-F3 artifact is not a Linux `Image` yet.  It is a
U-Boot-loaded ELF plus the existing riscv64 boot package:

- `build/riscv64/bpi-f3/bunixos-riscv64.elf`
- `build/riscv64/bpi-f3/bunix-riscv64.bootpkg`
- `build/riscv64/bpi-f3/boot-bunix-bpi-f3.cmd`
- `build/riscv64/bpi-f3/manifest.txt`

Build it with:

```sh
make riscv64-bpi-f3-artifacts
```

Check the host-side artifact shape with:

```sh
make test-riscv64-bpi-f3-artifacts
```

Prepare a mounted boot partition with:

```sh
make riscv64-bpi-f3-artifacts
tools/bpi-f3-smoke.sh --prepare /path/to/boot/partition
```

Print the U-Boot commands with:

```sh
tools/bpi-f3-smoke.sh --print-uboot
```

Verify a captured serial log with:

```sh
tools/bpi-f3-smoke.sh --check-log bpi-f3-serial.log
```

The initial U-Boot recipe is intentionally a manual command file rather than a
compiled boot script.  The first hardware sessions should run it command by
command so the operator can inspect `bdinfo`, `fdt print /chosen`, and memory
placement if the board firmware uses different load addresses.

## First Hardware Smoke

Operator setup:

- Use the vendor OpenSBI/U-Boot already proven to boot Linux on the board.
- Connect serial to the documented UART pins.  The BPI-F3 GPIO table names
  `R_UART0_TXD_3V3` and `R_UART0_RXD_3V3` on the 26-pin header.
- Copy the generated Bunix BPI-F3 artifacts to the first boot partition.
- Stop at the U-Boot prompt.
- Run the commands in `boot-bunix-bpi-f3.cmd` by hand.
- Capture the full serial log and run
  `tools/bpi-f3-smoke.sh --check-log bpi-f3-serial.log`.

Expected first milestone serial markers:

- `bunixos: riscv64 early bootstrap`
- `pmm: riscv64 ranges`
- `fdt: riscv64 cpus`
- `fdt: riscv64 timer`
- `fdt: riscv64 stdout`
- `fdt: riscv64 uart`
- `fdt: riscv64 interrupt-controller`
- `timer: riscv64 tick`
- `thread: riscv64 switch`
- `bootpkg: riscv64 initrd`
- `bootstrap-riscv64: online`
- `native: riscv64 server argc=1 argv0=/bin/abi-smoke.user`
- `musl hello argc=1 argv0=/bin/musl-hello`
- `machine: poweroff`

If the firmware FDT does not provide the same initrd properties as QEMU, fix
the U-Boot `/chosen` setup before adding kernel-side board constants.

## Follow-Up Hardware Work

After the first serial/poweroff smoke, bring up hardware in this order:

- Confirm that the firmware FDT properties discovered by the QEMU smoke also
  appear on BPI-F3: CPU hart IDs, `/cpus/timebase-frequency`,
  `/chosen/stdout-path`, UART-compatible nodes, interrupt-controller nodes,
  memory ranges, and `/chosen/linux,initrd-*`.
- Confirm the K1 UART binding and then add an early native UART backend under
  the riscv64 board layer.
- Confirm SBI timer and poweroff/reboot behavior on the vendor OpenSBI.
- Identify the interrupt-controller path from the vendor DT and BSP: PLIC,
  AIA, or vendor-specific routing.
- Decide whether the first SMP hardware milestone stays `SMP=1` or releases
  secondary harts.
- Split later device work into explorations for eMMC/microSD, USB, dual GbE,
  PCIe/M.2, display, WiFi/Bluetooth, and watchdog support.
