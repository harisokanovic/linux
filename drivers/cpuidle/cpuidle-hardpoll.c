// SPDX-License-Identifier: GPL-2.0
/*
 * hardpoll cpuidle driver: A simple cpuidle driver that always polls for
 * the next task, preventing CPUs from sleeping.
 *
 * For testing purposes only!
 *
 * Author: Haris Okanovic <harisokn@amazon.com>
 */

#include <linux/cpu.h>
#include <linux/cpuidle.h>
#include <linux/init.h>
#include <linux/module.h>
#include <linux/sched/clock.h>
#include <linux/sched/idle.h>

static struct cpuidle_device __percpu *hardpoll_cpuidle_devices;
static enum cpuhp_state hardpoll_hp_state;

static __cpuidle int hardpoll_idle(
	struct cpuidle_device *dev, struct cpuidle_driver *drv, int index)
{
	dev->poll_time_limit = false;

	raw_local_irq_enable();
	if (!current_set_polling_and_test()) {
		while (!need_resched())
		{ }
	}
	raw_local_irq_disable();

	current_clr_polling();

	return index;
}

static struct cpuidle_driver hardpoll_driver = {
	.name = "hardpoll",
	.governor = "hardpoll",
	.states = {
		{
			.enter = hardpoll_idle,
			.exit_latency = 1,
			.target_residency = 1,
			.power_usage = -1,
			.name = "hardpoll",
			.desc = "hardpoll",
		},
	},
	.safe_state_index = 0,
	.state_count = 1,
};

static int hardpoll_cpu_online(unsigned int cpu)
{
	struct cpuidle_device *dev;

	dev = per_cpu_ptr(hardpoll_cpuidle_devices, cpu);
	if (!dev->registered) {
		dev->cpu = cpu;
		if (cpuidle_register_device(dev)) {
			pr_notice("cpuidle_register_device %d failed!\n", cpu);
			return -EIO;
		}
	}

	return 0;
}

static int hardpoll_cpu_offline(unsigned int cpu)
{
	struct cpuidle_device *dev;

	dev = per_cpu_ptr(hardpoll_cpuidle_devices, cpu);
	if (dev->registered) {
		cpuidle_unregister_device(dev);
	}

	return 0;
}

static void hardpoll_uninit(void)
{
	if (hardpoll_hp_state) {
		cpuhp_remove_state(hardpoll_hp_state);
	}

	cpuidle_unregister_driver(&hardpoll_driver);

	free_percpu(hardpoll_cpuidle_devices);
	hardpoll_cpuidle_devices = NULL;
}

static int __init hardpoll_init(void)
{
	int ret;
	struct cpuidle_driver *drv = &hardpoll_driver;

	cpuidle_poll_state_init(drv);

	ret = cpuidle_register_driver(drv);
	if (ret < 0)
		return ret;

	hardpoll_cpuidle_devices = alloc_percpu(struct cpuidle_device);
	if (hardpoll_cpuidle_devices == NULL) {
		cpuidle_unregister_driver(drv);
		return -ENOMEM;
	}

	ret = cpuhp_setup_state(CPUHP_AP_ONLINE_DYN, "cpuidle/hardpoll:online",
				hardpoll_cpu_online, hardpoll_cpu_offline);
	if (ret < 0) {
		hardpoll_uninit();
	} else {
		hardpoll_hp_state = ret;
		ret = 0;
	}

	return ret;
}

static void __exit hardpoll_exit(void)
{
	hardpoll_uninit();
}

module_init(hardpoll_init);
module_exit(hardpoll_exit);
MODULE_DESCRIPTION("hardpoll cpuidle driver");
MODULE_LICENSE("GPL");
MODULE_AUTHOR("Haris Okanovic <harisokn@amazon.com>");
