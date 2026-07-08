# riscv64 BPI-F3 bringup notes

This document is the hardware bringup plan for Banana Pi BPI-F3.  It starts
only after the QEMU `virt` riscv64 userspace path is working end-to-end, so
board debugging can focus on firmware, FDT, console, timer, interrupt, and
shutdown behavior.  If the emulator path regresses, pause BPI-F3 hardware work
and repair QEMU `virt` first.

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

Before preparing hardware media or collecting a BPI-F3 serial log, run the
emulator gate:

```sh
make test-riscv64-bpi-f3-emulator-gate
```

That target runs the QEMU `virt` riscv64 userspace smoke, validates the
resulting serial log with the BPI-F3 smoke checker, and classifies the emulator
evidence.  Do not proceed to physical-board work if this gate fails.

The local QEMU build does not provide a SpacemiT K1 or Banana Pi BPI-F3 machine
model.  Until one exists, `virt` is the emulator surrogate for generic riscv64
boot, userspace, FDT parsing, and evidence-tool validation; BPI-F3-specific
proof still comes from a later physical-board serial log.

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
tools/bpi-f3-smoke.sh --review-log bpi-f3-serial.log
```

The review command runs the checks below and then prints the classifier and
summary tables.  The individual commands are also available when debugging a
specific phase.  The Bunix smoke check also validates that memory size is
nonzero, kernel/initrd/FDT ranges are ordered, and initrd/FDT size fields match
their start/end ranges.

The classifier includes conservative `first-hardware-milestone` and
`second-hardware-milestone` rows.  Treat those rows as review evidence only;
the exploration tasks should still be closed manually after inspecting the full
physical-board serial capture.

```sh
tools/bpi-f3-smoke.sh --check-preboot-log bpi-f3-serial.log
tools/bpi-f3-smoke.sh --check-log bpi-f3-serial.log
tools/bpi-f3-smoke.sh --classify-log bpi-f3-serial.log
tools/bpi-f3-smoke.sh --summarize-log bpi-f3-serial.log
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
  `tools/bpi-f3-smoke.sh --review-log bpi-f3-serial.log`.
- Record the review output:
  which exploration tasks have supporting evidence, which still need follow-up,
  and the summary values listed below.
- In particular, record the
  preboot model/compatible strings, CPU count, timebase, firmware stdout path,
  resolved UART, UART MMIO base, UART count,
  interrupt-controller path/compatible/count, and selected interrupt-routing
  path/compatible in the exploration notes.

The command recipe intentionally prints U-Boot `bdinfo`, root `model` and
`compatible`, `/chosen`, `/aliases`, and `/cpus` before entering Bunix.  Those
preboot diagnostics give the board-specific evidence needed to identify the
firmware DT and compare its handoff with the Bunix FDT parser output.

Expected first milestone serial markers:

- `bunixos: riscv64 early bootstrap`
- `pmm: riscv64 ranges`
- `boot: riscv64 memory-base=...`
- `boot: riscv64 memory-size=...`
- `boot: riscv64 kernel-start=...`
- `boot: riscv64 kernel-end=...`
- `boot: riscv64 initrd-start=...`
- `boot: riscv64 initrd-end=...`
- `boot: riscv64 initrd-size=...`
- `boot: riscv64 fdt-start=...`
- `boot: riscv64 fdt-end=...`
- `boot: riscv64 fdt-size=...`
- `fdt: riscv64 cpus`
- `fdt: riscv64 cpu-count=...`
- `smp: riscv64 discovered-harts=...`
- `smp: riscv64 started-harts=...`
- `smp: riscv64 boot-hart=...`
- `smp: riscv64 secondary-policy=parked`
- `fdt: riscv64 timer`
- `fdt: riscv64 timebase-hz=...`
- `fdt: riscv64 stdout`
- `fdt: riscv64 stdout-path=...`
- `fdt: riscv64 stdout-uart`
- `fdt: riscv64 stdout-resolved=...`
- `fdt: riscv64 stdout-uart-base=...`
- `fdt: riscv64 uart`
- `fdt: riscv64 uart-count=...`
- `fdt: riscv64 interrupt-controller`
- `fdt: riscv64 interrupt-controller-path=...`
- `fdt: riscv64 interrupt-controller-compatible=...`
- `fdt: riscv64 interrupt-controller-count=...`
- `fdt: riscv64 interrupt-routing-path=...`
- `fdt: riscv64 interrupt-routing-compatible=...`
- `timer: riscv64 tick`
- `thread: riscv64 switch`
- `bootpkg: riscv64 initrd`
- `bootstrap-riscv64: online`
- `native: riscv64 server argc=1 argv0=/bin/abi-smoke.user`
- `musl hello argc=1 argv0=/bin/musl-hello`
- `machine: poweroff`
- `sbi: system reset poweroff`

If the firmware FDT does not provide the same initrd properties as QEMU, fix
the U-Boot `/chosen` setup before adding kernel-side board constants.

The `fdt: riscv64 ...=...` diagnostic lines are part of the evidence package
for the still-open hardware tasks: they identify the board CPU count,
timebase, selected stdout UART, UART MMIO base, and interrupt-controller path
from the firmware DT.

The classifier output is tab-separated and intentionally conservative.  It
reports `evidence` only when the captured log contains the markers needed to
support a task; final task completion still requires reviewing the actual
serial log and updating the exploration notes.

The summary output is also tab-separated.  It extracts the board-specific
values that must be reviewed before closing the remaining hardware tasks:
`memory-base`, `memory-size`, `kernel-start`, `kernel-end`, `initrd-start`,
`initrd-end`, `initrd-size`, `fdt-start`, `fdt-end`, `fdt-size`,
`cpu-count`, `smp-discovered-harts`, `smp-started-harts`, `smp-boot-hart`,
`smp-secondary-policy`, `timebase-hz`, `stdout-path`, `stdout-resolved`,
`stdout-uart-base`, `uart-count`, `interrupt-controller-path`,
`interrupt-controller-compatible`, `interrupt-controller-count`,
`interrupt-routing-path`, and `interrupt-routing-compatible`.

## Follow-Up Hardware Work

After the first serial/poweroff smoke, bring up hardware in this order:

- Confirm that the firmware FDT properties discovered by the QEMU smoke also
  appear on BPI-F3: CPU hart IDs, `/cpus/timebase-frequency`,
  `/chosen/stdout-path`, UART-compatible nodes, interrupt-controller nodes,
  memory ranges, and `/chosen/linux,initrd-*`.
- Confirm the K1 UART binding and then add an early native UART backend under
  the riscv64 board layer.  The generic riscv64 FDT scanner now resolves
  `/chosen/stdout-path`, including `/aliases` entries such as `serial0`, to a
  discovered UART node so the board run can identify the intended console
  device before native MMIO output is enabled.
- Confirm SBI timer and poweroff/reboot behavior on the vendor OpenSBI.  The
  riscv64 power path now prefers the SBI System Reset extension and falls back
  to legacy shutdown if that extension returns.
- Identify the interrupt-controller path from the vendor DT and BSP: PLIC,
  AIA, or vendor-specific routing.
- Keep the first SMP hardware milestone at one started hart while firmware
  hart discovery is proved.  Real BPI-F3 logs should show
  `smp-secondary-policy=parked`; secondary hart release belongs in a later
  SMP bringup slice after the single-hart board boot is reliable.
- Split later device work into explorations for eMMC/microSD, USB, dual GbE,
  PCIe/M.2, display, WiFi/Bluetooth, and watchdog support.

Follow-up exploration files:

- `054-bpi-f3-emmc-microsd-storage.txt`
- `055-bpi-f3-usb-host-otg.txt`
- `056-bpi-f3-dual-gbe-network.txt`
- `057-bpi-f3-pcie-m2.txt`
- `058-bpi-f3-display-hdmi.txt`
- `059-bpi-f3-wifi-bluetooth.txt`
- `060-bpi-f3-watchdog-reset.txt`
