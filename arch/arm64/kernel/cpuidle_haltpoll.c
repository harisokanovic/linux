/* SPDX-License-Identifier: GPL-2.0 */

#include <asm/cpuidle_haltpoll.h>
#include <linux/acpi.h>

#ifdef CONFIG_ACPI

static bool awsnitro_detect_via_acpi(void)
{
	if (acpi_disabled)
		return false;

	/* Hypervisor ID is only available in ACPI v6+ */
	if (acpi_gbl_FADT.header.revision < 6)
		return false;

	if (strncmp((char *)&acpi_gbl_FADT.hypervisor_id, "AWSNITRO", 8) != 0) {
		return false;
	}

	pr_info("Detected AWSNITRO hypervisor\n");
	return true;
}

#else

static inline bool awsnitro_detect_via_acpi(void)
{
	return false;
}

#endif

bool arch_haltpoll_want(bool force)
{
	/*
	 * Enable by default on AWS Nitro Graviton instances.
	 * Enabling on KVM requires support for arch_haltpoll_enable() and
	 * arch_haltpoll_disable(), which is presently missing on Arm.
	 */
	return awsnitro_detect_via_acpi() || force;
}

EXPORT_SYMBOL_GPL(arch_haltpoll_want);
