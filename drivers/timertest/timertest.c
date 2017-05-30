#include <linux/kernel.h>
#include <linux/module.h>

#define CHECK_TIMER_RATE 2000 /* ms */
#define TEST_TIMER_CNT 1000

struct timer_list check_timer;

struct timer_list test_timer[TEST_TIMER_CNT];
unsigned long test_timer_calls[TEST_TIMER_CNT];
unsigned long test_timer_calls_old[TEST_TIMER_CNT];

static void callback_check(unsigned long data)
{
	int i;
	printk("Check timer.");

	for (i=0; i<TEST_TIMER_CNT; i++) {
		if (test_timer_calls[i] == test_timer_calls_old[i]) {
			printk("Stalled: %d (%d)\n", i, (5+i%100));
		}

		test_timer_calls_old[i] = test_timer_calls[i];
	}

	mod_timer(&check_timer, jiffies + msecs_to_jiffies(CHECK_TIMER_RATE));
}


static void callback_test(unsigned long i)
{
	test_timer_calls[i]++;
	mod_timer(&test_timer[i], jiffies + msecs_to_jiffies(5 + i % 100));
}

static int __init timertest_init(void)
{
	int i;

	for (i=0; i<TEST_TIMER_CNT; i++) {
		setup_timer(&test_timer[i], callback_test, i);
		mod_timer(&test_timer[i], jiffies + msecs_to_jiffies(5 + i % 100));
	}
	setup_timer(&check_timer, callback_check, 0);
	mod_timer(&check_timer, jiffies + msecs_to_jiffies(CHECK_TIMER_RATE));
   return 0;
}

static void __exit timertest_exit(void)
{
	int i;

	del_timer_sync(&check_timer);
	for (i=0; i<TEST_TIMER_CNT; i++) {
		del_timer_sync(&test_timer[i]);
	}
}

module_init(timertest_init);
module_exit(timertest_exit);
MODULE_LICENSE("GPL");
