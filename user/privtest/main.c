#include <bunix/syscall.h>

#include <stdio.h>

enum {
	VIRTIO_BLK_BAR0 = (6ull << 8),
};

static int expect_denied(const char *name, long value)
{
	if (value != -1) {
		fprintf(stderr, "privtest %s unexpectedly returned %ld\n",
			name, value);
		return -1;
	}
	printf("privtest %s denied\n", name);
	return 0;
}

int main(void)
{
	int failed = 0;

	if (expect_denied("power-null", bunix_machine_poweroff(0)) != 0) {
		failed = 1;
	}
	if (expect_denied("power-public-handle",
			  bunix_machine_poweroff(BUNIX_HANDLE_POWER_AUTH)) != 0) {
		failed = 1;
	}
	if (expect_denied("pci-bar-null",
			  bunix_hw_pci_bar_grant(0, VIRTIO_BLK_BAR0, 0, 4,
						 BUNIX_DEV_OP_READ)) != 0) {
		failed = 1;
	}
	if (expect_denied("pci-bar-public-handle",
			  bunix_hw_pci_bar_grant(BUNIX_HANDLE_PCI_AUTH,
						 VIRTIO_BLK_BAR0, 0, 4,
						 BUNIX_DEV_OP_READ)) != 0) {
		failed = 1;
	}
	if (expect_denied("pci-irq-null",
			  bunix_hw_pci_irq_grant(0, VIRTIO_BLK_BAR0, 0)) != 0) {
		failed = 1;
	}
	if (expect_denied("pci-irq-public-handle",
			  bunix_hw_pci_irq_grant(BUNIX_HANDLE_PCI_AUTH,
						 VIRTIO_BLK_BAR0, 0)) != 0) {
		failed = 1;
	}

	if (failed != 0) {
		return 1;
	}
	printf("privtest ok\n");
	return 0;
}
