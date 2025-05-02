/* SPDX-License-Identifier: GPL-2.0-or-later */
/*
 * Generic barrier definitions.
 *
 * It should be possible to use these on really simple architectures,
 * but it serves more as a starting point for new ports.
 *
 * Copyright (C) 2007 Red Hat, Inc. All Rights Reserved.
 * Written by David Howells (dhowells@redhat.com)
 */
#ifndef __ASM_GENERIC_BARRIER_H
#define __ASM_GENERIC_BARRIER_H

#ifndef __ASSEMBLY__

#include <linux/compiler.h>
#include <linux/kcsan-checks.h>
#include <linux/minmax.h>
#include <asm/rwonce.h>

#ifndef nop
#define nop()	asm volatile ("nop")
#endif

/*
 * Architectures that want generic instrumentation can define __ prefixed
 * variants of all barriers.
 */

#ifdef __mb
#define mb()	do { kcsan_mb(); __mb(); } while (0)
#endif

#ifdef __rmb
#define rmb()	do { kcsan_rmb(); __rmb(); } while (0)
#endif

#ifdef __wmb
#define wmb()	do { kcsan_wmb(); __wmb(); } while (0)
#endif

#ifdef __dma_mb
#define dma_mb()	do { kcsan_mb(); __dma_mb(); } while (0)
#endif

#ifdef __dma_rmb
#define dma_rmb()	do { kcsan_rmb(); __dma_rmb(); } while (0)
#endif

#ifdef __dma_wmb
#define dma_wmb()	do { kcsan_wmb(); __dma_wmb(); } while (0)
#endif

/*
 * Force strict CPU ordering. And yes, this is required on UP too when we're
 * talking to devices.
 *
 * Fall back to compiler barriers if nothing better is provided.
 */

#ifndef mb
#define mb()	barrier()
#endif

#ifndef rmb
#define rmb()	mb()
#endif

#ifndef wmb
#define wmb()	mb()
#endif

#ifndef dma_mb
#define dma_mb()	mb()
#endif

#ifndef dma_rmb
#define dma_rmb()	rmb()
#endif

#ifndef dma_wmb
#define dma_wmb()	wmb()
#endif

#ifndef __smp_mb
#define __smp_mb()	mb()
#endif

#ifndef __smp_rmb
#define __smp_rmb()	rmb()
#endif

#ifndef __smp_wmb
#define __smp_wmb()	wmb()
#endif

#ifdef CONFIG_SMP

#ifndef smp_mb
#define smp_mb()	do { kcsan_mb(); __smp_mb(); } while (0)
#endif

#ifndef smp_rmb
#define smp_rmb()	do { kcsan_rmb(); __smp_rmb(); } while (0)
#endif

#ifndef smp_wmb
#define smp_wmb()	do { kcsan_wmb(); __smp_wmb(); } while (0)
#endif

#else	/* !CONFIG_SMP */

#ifndef smp_mb
#define smp_mb()	barrier()
#endif

#ifndef smp_rmb
#define smp_rmb()	barrier()
#endif

#ifndef smp_wmb
#define smp_wmb()	barrier()
#endif

#endif	/* CONFIG_SMP */

#ifndef __smp_store_mb
#define __smp_store_mb(var, value)  do { WRITE_ONCE(var, value); __smp_mb(); } while (0)
#endif

#ifndef __smp_mb__before_atomic
#define __smp_mb__before_atomic()	__smp_mb()
#endif

#ifndef __smp_mb__after_atomic
#define __smp_mb__after_atomic()	__smp_mb()
#endif

#ifndef __smp_store_release
#define __smp_store_release(p, v)					\
do {									\
	compiletime_assert_atomic_type(*p);				\
	__smp_mb();							\
	WRITE_ONCE(*p, v);						\
} while (0)
#endif

#ifndef __smp_load_acquire
#define __smp_load_acquire(p)						\
({									\
	__unqual_scalar_typeof(*p) ___p1 = READ_ONCE(*p);		\
	compiletime_assert_atomic_type(*p);				\
	__smp_mb();							\
	(typeof(*p))___p1;						\
})
#endif

#ifdef CONFIG_SMP

#ifndef smp_store_mb
#define smp_store_mb(var, value)  do { kcsan_mb(); __smp_store_mb(var, value); } while (0)
#endif

#ifndef smp_mb__before_atomic
#define smp_mb__before_atomic()	do { kcsan_mb(); __smp_mb__before_atomic(); } while (0)
#endif

#ifndef smp_mb__after_atomic
#define smp_mb__after_atomic()	do { kcsan_mb(); __smp_mb__after_atomic(); } while (0)
#endif

#ifndef smp_store_release
#define smp_store_release(p, v) do { kcsan_release(); __smp_store_release(p, v); } while (0)
#endif

#ifndef smp_load_acquire
#define smp_load_acquire(p) __smp_load_acquire(p)
#endif

#else	/* !CONFIG_SMP */

#ifndef smp_store_mb
#define smp_store_mb(var, value)  do { WRITE_ONCE(var, value); barrier(); } while (0)
#endif

#ifndef smp_mb__before_atomic
#define smp_mb__before_atomic()	barrier()
#endif

#ifndef smp_mb__after_atomic
#define smp_mb__after_atomic()	barrier()
#endif

#ifndef smp_store_release
#define smp_store_release(p, v)						\
do {									\
	barrier();							\
	WRITE_ONCE(*p, v);						\
} while (0)
#endif

#ifndef smp_load_acquire
#define smp_load_acquire(p)						\
({									\
	__unqual_scalar_typeof(*p) ___p1 = READ_ONCE(*p);		\
	barrier();							\
	(typeof(*p))___p1;						\
})
#endif

#endif	/* CONFIG_SMP */

/* Barriers for virtual machine guests when talking to an SMP host */
#define virt_mb() do { kcsan_mb(); __smp_mb(); } while (0)
#define virt_rmb() do { kcsan_rmb(); __smp_rmb(); } while (0)
#define virt_wmb() do { kcsan_wmb(); __smp_wmb(); } while (0)
#define virt_store_mb(var, value) do { kcsan_mb(); __smp_store_mb(var, value); } while (0)
#define virt_mb__before_atomic() do { kcsan_mb(); __smp_mb__before_atomic(); } while (0)
#define virt_mb__after_atomic()	do { kcsan_mb(); __smp_mb__after_atomic(); } while (0)
#define virt_store_release(p, v) do { kcsan_release(); __smp_store_release(p, v); } while (0)
#define virt_load_acquire(p) __smp_load_acquire(p)

/**
 * smp_acquire__after_ctrl_dep() - Provide ACQUIRE ordering after a control dependency
 *
 * A control dependency provides a LOAD->STORE order, the additional RMB
 * provides LOAD->LOAD order, together they provide LOAD->{LOAD,STORE} order,
 * aka. (load)-ACQUIRE.
 *
 * Architectures that do not do load speculation can have this be barrier().
 */
#ifndef smp_acquire__after_ctrl_dep
#define smp_acquire__after_ctrl_dep()		smp_rmb()
#endif

/**
 * smp_cond_load_relaxed() - (Spin) wait for cond with no ordering guarantees
 * @ptr: pointer to the variable to wait on
 * @cond: boolean expression to wait for
 *
 * Equivalent to using READ_ONCE() on the condition variable.
 *
 * Due to C lacking lambda expressions we load the value of *ptr into a
 * pre-named variable @VAL to be used in @cond.
 */
#ifndef smp_cond_load_relaxed
#define smp_cond_load_relaxed(ptr, cond_expr) ({		\
	typeof(ptr) __PTR = (ptr);				\
	__unqual_scalar_typeof(*ptr) VAL;			\
	for (;;) {						\
		VAL = READ_ONCE(*__PTR);			\
		if (cond_expr)					\
			break;					\
		cpu_relax();					\
	}							\
	(typeof(*ptr))VAL;					\
})
#endif

/**
 * smp_cond_load_acquire() - (Spin) wait for cond with ACQUIRE ordering
 * @ptr: pointer to the variable to wait on
 * @cond: boolean expression to wait for
 *
 * Equivalent to using smp_load_acquire() on the condition variable but employs
 * the control dependency of the wait to reduce the barrier on many platforms.
 */
#ifndef smp_cond_load_acquire
#define smp_cond_load_acquire(ptr, cond_expr) ({		\
	__unqual_scalar_typeof(*ptr) _val;			\
	_val = smp_cond_load_relaxed(ptr, cond_expr);		\
	smp_acquire__after_ctrl_dep();				\
	(typeof(*ptr))_val;					\
})
#endif

#ifndef SMP_TIMEWAIT_SPIN_BASE
#define SMP_TIMEWAIT_SPIN_BASE         16
#endif

static inline u64 ___cond_spinwait(u64 now, u64 prev, u64 end,
                                  u32 *spin, bool *wait, u64 slack)
{
       if (now >= end)
               return 0;

       *wait = false;

       /*
        * Scale the spin-count up or down so we evaluate the time-expr every
        * slack unit of time or so.
        */
       if ((now - prev) < slack)
               *spin <<= 1;
       else
               /*
                * Ensure the spin-count is at least SMP_TIMEWAIT_SPIN_BASE
                * when scaling down to guard against artificially low values
                * due to interrupts etc. Clamping down also handles the case
                * of the first iteration (*spin == 0).
                */
               *spin = max((*spin >> 1) + (*spin >> 2), SMP_TIMEWAIT_SPIN_BASE);

       return now;
}

#ifndef SMP_TIMEWAIT_SLACK_FINE_US
#define SMP_TIMEWAIT_SLACK_FINE_US     2UL
#endif

#ifndef SMP_TIMEWAIT_SLACK_COARSE_US
#define SMP_TIMEWAIT_SLACK_COARSE_US   5UL
#endif

/*
 * wait_policy: to minimize how often we do the (typically) expensive
 * time-check, expect a slack duration which would vary based on
 * architecture.
 *
 * For the generic variant, the fine and coarse variants have a slack
 * duration of SMP_TIMEWAIT_SLACK_FINE_US and SMP_TIMEWAIT_SLACK_COARSE_US.
 */
#ifndef __smp_cond_timewait_fine
#define __smp_cond_timewait_fine(now, prev, end, spin, wait)   \
       ___cond_spinwait(now, prev, end, spin, wait,            \
                           SMP_TIMEWAIT_SLACK_FINE_US)
#endif

#ifndef __smp_cond_timewait_coarse
#define __smp_cond_timewait_coarse(now, prev, end, spin, wait) \
       ___cond_spinwait(now, prev, end, spin, wait,            \
                           SMP_TIMEWAIT_SLACK_COARSE_US)
#endif

/*
 * Non-spin primitive that allows waiting for stores to an address,
 * with support for a timeout. This works in conjunction with an
 * architecturally defined wait_policy.
 */
#ifndef __smp_timewait_store
#define __smp_timewait_store(ptr, val) do { } while (0)
#endif

#ifndef __smp_cond_load_relaxed_timewait
#define __smp_cond_load_relaxed_timewait(ptr, cond_expr, wait_policy,  \
                                        time_expr, time_end) ({        \
       typeof(ptr) __PTR = (ptr);                                      \
       __unqual_scalar_typeof(*ptr) VAL;                               \
       u32 __n = 0, __spin = 0;                                        \
       u64 __prev = 0, __end = (time_end);                             \
       bool __wait = false;                                            \
                                                                       \
       for (;;) {                                                      \
               VAL = READ_ONCE(*__PTR);                                \
               if (cond_expr)                                          \
                       break;                                          \
               cpu_relax();                                            \
               if (++__n < __spin)                                     \
                       continue;                                       \
               if (!(__prev = wait_policy((time_expr), __prev, __end,  \
                                         &__spin, &__wait)))           \
                       break;                                          \
               if (__wait)                                             \
                       __smp_timewait_store(__PTR, VAL);               \
               __n = 0;                                                \
       }                                                               \
       (typeof(*ptr))VAL;                                              \
})
#endif

/**
 * smp_cond_load_relaxed_timewait() - (Spin) wait for cond with no ordering
 * guarantees until a timeout expires.
 * @ptr: pointer to the variable to wait on
 * @cond: boolean expression to wait for
 * @wait_policy: policy handler that adjusts the number of times we spin or
 *  wait for cacheline to change (depends on architecture, not supported in
 *  generic code.) before evaluating the time-expr.
 * @time_expr: monotonic expression that evaluates to the current time
 * @time_end: compared against time_expr
 *
 * The default policies (__smp_cond_timewait_coarse, __smp_cond_timewait_fine)
 * assume that time_expr and time_end evaluate to time in us (both with a user
 * specified precision.)
 * With a user specified policy, any units and precision can be used.
 *
 * Equivalent to using READ_ONCE() on the condition variable.
 */
#define smp_cond_load_relaxed_timewait(ptr, cond_expr, wait_policy,    \
                                        time_expr, time_end) ({        \
       __unqual_scalar_typeof(*ptr) _val;;                             \
       BUILD_BUG_ON_MSG(!__same_type(typeof(time_expr), u64),          \
                        "incompatible time units");                    \
       _val = __smp_cond_load_relaxed_timewait(ptr, cond_expr,         \
                                             wait_policy, time_expr,   \
                                             time_end);                \
       (typeof(*ptr))_val;                                             \
})

/**
 * smp_cond_load_acquire_timewait() - (Spin) wait for cond with ACQUIRE ordering
 * until a timeout expires.
 * @ptr: pointer to the variable to wait on
 * @cond: boolean expression to wait for
 * @wait_policy: policy handler that adjusts how much we spin before evaluating
 *  the timeout, and if we drop into a wait for cacheline to change (depending
 *  on architecture support.)
 * @time_expr: monotonic expression that evaluates to the current time
 * @time_end: compared against time_expr
 *
 * Equivalent to using smp_cond_load_acquire() on the condition variable with
 * a timeout.
 */
#ifndef smp_cond_load_acquire_timewait
#define smp_cond_load_acquire_timewait(ptr, cond_expr, wait_policy,    \
                                      time_expr, time_end) ({          \
       __unqual_scalar_typeof(*ptr) _val;                              \
       _val = smp_cond_load_relaxed_timewait(ptr, cond_expr,           \
                                             wait_policy, time_expr,   \
                                             time_end);                \
       /* Depends on the control dependency of the wait above. */      \
       smp_acquire__after_ctrl_dep();                                  \
       (typeof(*ptr))_val;                                             \
})
#endif
/*
 * pmem_wmb() ensures that all stores for which the modification
 * are written to persistent storage by preceding instructions have
 * updated persistent storage before any data  access or data transfer
 * caused by subsequent instructions is initiated.
 */
#ifndef pmem_wmb
#define pmem_wmb()	wmb()
#endif

/*
 * ioremap_wc() maps I/O memory as memory with write-combining attributes. For
 * this kind of memory accesses, the CPU may wait for prior accesses to be
 * merged with subsequent ones. In some situation, such wait is bad for the
 * performance. io_stop_wc() can be used to prevent the merging of
 * write-combining memory accesses before this macro with those after it.
 */
#ifndef io_stop_wc
#define io_stop_wc() do { } while (0)
#endif

/*
 * Architectures that guarantee an implicit smp_mb() in switch_mm()
 * can override smp_mb__after_switch_mm.
 */
#ifndef smp_mb__after_switch_mm
# define smp_mb__after_switch_mm()	smp_mb()
#endif

#endif /* !__ASSEMBLY__ */
#endif /* __ASM_GENERIC_BARRIER_H */
