// SPDX-License-Identifier: GPL-2.0
/*
 * ARM64 CPU idle driver using wfe polling
 *
 * Copyright 2024 Amazon.com, Inc. or its affiliates. All rights reserved.
 *
 * Authors:
 *   Haris Okanovic <harisokn@amazon.com>
 *   Brian Silver <silverbr@amazon.com>
 *
 * Based on cpuidle-arm.c
 * Copyright (C) 2014 ARM Ltd.
 * Author: Lorenzo Pieralisi <lorenzo.pieralisi@arm.com>
 */

#include <linux/cpu.h>
#include <linux/cpu_cooling.h>
#include <linux/cpuidle.h>
#include <linux/sched/clock.h>

#include <asm/cpuidle.h>
#include <asm/readex.h>

#include "dt_idle_states.h"

/* Max duration of the wfe() poll loop in us, before transitioning to
 * arch_cpu_idle()/wfi() sleep.
 */
#define DEFAULT_POLL_LIMIT_US 100
static unsigned int poll_limit __read_mostly = DEFAULT_POLL_LIMIT_US;

/*
 * arm_idle_wfe_poll - Polls state in wfe loop until reschedule is
 * needed or timeout
 */
static int __cpuidle arm_idle_wfe_poll(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int idx)
{
	u64 time_start, time_limit;

	time_start = local_clock();
	dev->poll_time_limit = false;

	local_irq_enable();

	if (current_set_polling_and_test())
		goto end;

	time_limit = cpuidle_poll_time(drv, dev);

	do {
		// exclusive read arms the monitor for wfe
		if (__READ_ONCE_EX(current_thread_info()->flags) & _TIF_NEED_RESCHED)
			goto end;

		// may exit prematurely, see ARM_ARCH_TIMER_EVTSTREAM
		wfe();
	} while (local_clock() - time_start < time_limit);

	dev->poll_time_limit = true;

end:
	current_clr_polling();
	return idx;
}

/*
 * arm_idle_wfi - Places cpu in lower power state until interrupt,
 * a fallback to polling
 */
static int __cpuidle arm_idle_wfi(struct cpuidle_device *dev,
				struct cpuidle_driver *drv, int idx)
{
	if (current_clr_polling_and_test()) {
		local_irq_enable();
		return idx;
	}
	arch_cpu_idle();
	return idx;
}

static struct cpuidle_driver arm_poll_idle_driver __initdata = {
	.name = "arm_poll_idle",
	.owner = THIS_MODULE,
	.states = {
		{
			.enter			= arm_idle_wfe_poll,
			.exit_latency		= 0,
			.target_residency	= 0,
			.exit_latency_ns	= 0,
			.power_usage		= UINT_MAX,
			.flags			= CPUIDLE_FLAG_POLLING,
			.name			= "WFE",
			.desc			= "ARM WFE",
		},
		{
			.enter			= arm_idle_wfi,
			.exit_latency		= DEFAULT_POLL_LIMIT_US,
			.target_residency	= DEFAULT_POLL_LIMIT_US,
			.power_usage		= UINT_MAX,
			.name			= "WFI",
			.desc			= "ARM WFI",
		},
	},
	.state_count = 2,
};

/*
 * arm_poll_init_cpu - Initializes arm cpuidle polling driver for one cpu
 */
static int __init arm_poll_init_cpu(int cpu)
{
	int ret;
	struct cpuidle_driver *drv;

	drv = kmemdup(&arm_poll_idle_driver, sizeof(*drv), GFP_KERNEL);
	if (!drv)
		return -ENOMEM;

	drv->cpumask = (struct cpumask *)cpumask_of(cpu);
	drv->states[1].exit_latency = poll_limit;
	drv->states[1].target_residency = poll_limit;

	ret = cpuidle_register(drv, NULL);
	if (ret) {
		pr_err("failed to register driver: %d, cpu %d\n", ret, cpu);
		goto out_kfree_drv;
	}

	pr_info("registered driver cpu %d\n", cpu);

	cpuidle_cooling_register(drv);

	return 0;

out_kfree_drv:
	kfree(drv);
	return ret;
}

/*
 * arm_poll_init - Initializes arm cpuidle polling driver
 */
static int __init arm_poll_init(void)
{
	int cpu, ret;
	struct cpuidle_driver *drv;
	struct cpuidle_device *dev;

	for_each_possible_cpu(cpu) {
		ret = arm_poll_init_cpu(cpu);
		if (ret)
			goto out_fail;
	}

	return 0;

out_fail:
	pr_info("de-register all");
	while (--cpu >= 0) {
		dev = per_cpu(cpuidle_devices, cpu);
		drv = cpuidle_get_cpu_driver(dev);
		cpuidle_unregister(drv);
		kfree(drv);
	}

	return ret;
}

module_param(poll_limit, uint, 0444);
device_initcall(arm_poll_init);
