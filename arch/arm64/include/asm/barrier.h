/* SPDX-License-Identifier: GPL-2.0-only */
/*
 * Based on arch/arm/include/asm/barrier.h
 *
 * Copyright (C) 2012 ARM Ltd.
 */
#ifndef __ASM_BARRIER_H
#define __ASM_BARRIER_H

#ifndef __ASSEMBLY__

#include <linux/kasan-checks.h>
#include <linux/minmax.h>

#include <asm/alternative-macros.h>

#define __nops(n)	".rept	" #n "\nnop\n.endr\n"
#define nops(n)		asm volatile(__nops(n))

#define sev()		asm volatile("sev" : : : "memory")
#define wfe()		asm volatile("wfe" : : : "memory")
#define wfet(val)	asm volatile("msr s0_3_c1_c0_0, %0"	\
				     : : "r" (val) : "memory")
#define wfi()		asm volatile("wfi" : : : "memory")
#define wfit(val)	asm volatile("msr s0_3_c1_c0_1, %0"	\
				     : : "r" (val) : "memory")

#define isb()		asm volatile("isb" : : : "memory")
#define dmb(opt)	asm volatile("dmb " #opt : : : "memory")
#define dsb(opt)	asm volatile("dsb " #opt : : : "memory")

#define psb_csync()	asm volatile("hint #17" : : : "memory")
#define __tsb_csync()	asm volatile("hint #18" : : : "memory")
#define csdb()		asm volatile("hint #20" : : : "memory")

/*
 * Data Gathering Hint:
 * This instruction prevents merging memory accesses with Normal-NC or
 * Device-GRE attributes before the hint instruction with any memory accesses
 * appearing after the hint instruction.
 */
#define dgh()		asm volatile("hint #6" : : : "memory")

#define spec_bar()	asm volatile(ALTERNATIVE("dsb nsh\nisb\n",		\
						 SB_BARRIER_INSN"nop\n",	\
						 ARM64_HAS_SB))

#ifdef CONFIG_ARM64_PSEUDO_NMI
#define pmr_sync()						\
	do {							\
		asm volatile(					\
		ALTERNATIVE_CB("dsb sy",			\
			       ARM64_HAS_GIC_PRIO_RELAXED_SYNC,	\
			       alt_cb_patch_nops)		\
		);						\
	} while(0)
#else
#define pmr_sync()	do {} while (0)
#endif

#define __mb()		dsb(sy)
#define __rmb()		dsb(ld)
#define __wmb()		dsb(st)

#define __dma_mb()	dmb(osh)
#define __dma_rmb()	dmb(oshld)
#define __dma_wmb()	dmb(oshst)

#define io_stop_wc()	dgh()

#define tsb_csync()								\
	do {									\
		/*								\
		 * CPUs affected by Arm Erratum 2054223 or 2067961 needs	\
		 * another TSB to ensure the trace is flushed. The barriers	\
		 * don't have to be strictly back to back, as long as the	\
		 * CPU is in trace prohibited state.				\
		 */								\
		if (cpus_have_final_cap(ARM64_WORKAROUND_TSB_FLUSH_FAILURE))	\
			__tsb_csync();						\
		__tsb_csync();							\
	} while (0)

/*
 * Generate a mask for array_index__nospec() that is ~0UL when 0 <= idx < sz
 * and 0 otherwise.
 */
#define array_index_mask_nospec array_index_mask_nospec
static inline unsigned long array_index_mask_nospec(unsigned long idx,
						    unsigned long sz)
{
	unsigned long mask;

	asm volatile(
	"	cmp	%1, %2\n"
	"	sbc	%0, xzr, xzr\n"
	: "=r" (mask)
	: "r" (idx), "Ir" (sz)
	: "cc");

	csdb();
	return mask;
}

/*
 * Ensure that reads of the counter are treated the same as memory reads
 * for the purposes of ordering by subsequent memory barriers.
 *
 * This insanity brought to you by speculative system register reads,
 * out-of-order memory accesses, sequence locks and Thomas Gleixner.
 *
 * https://lore.kernel.org/r/alpine.DEB.2.21.1902081950260.1662@nanos.tec.linutronix.de/
 */
#define arch_counter_enforce_ordering(val) do {				\
	u64 tmp, _val = (val);						\
									\
	asm volatile(							\
	"	eor	%0, %1, %1\n"					\
	"	add	%0, sp, %0\n"					\
	"	ldr	xzr, [%0]"					\
	: "=r" (tmp) : "r" (_val));					\
} while (0)

#define __smp_mb()	dmb(ish)
#define __smp_rmb()	dmb(ishld)
#define __smp_wmb()	dmb(ishst)

#define __smp_store_release(p, v)					\
do {									\
	typeof(p) __p = (p);						\
	union { __unqual_scalar_typeof(*p) __val; char __c[1]; } __u =	\
		{ .__val = (__force __unqual_scalar_typeof(*p)) (v) };	\
	compiletime_assert_atomic_type(*p);				\
	kasan_check_write(__p, sizeof(*p));				\
	switch (sizeof(*p)) {						\
	case 1:								\
		asm volatile ("stlrb %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u8 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 2:								\
		asm volatile ("stlrh %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u16 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 4:								\
		asm volatile ("stlr %w1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u32 *)__u.__c)		\
				: "memory");				\
		break;							\
	case 8:								\
		asm volatile ("stlr %x1, %0"				\
				: "=Q" (*__p)				\
				: "rZ" (*(__u64 *)__u.__c)		\
				: "memory");				\
		break;							\
	}								\
} while (0)

#define __smp_load_acquire(p)						\
({									\
	union { __unqual_scalar_typeof(*p) __val; char __c[1]; } __u;	\
	typeof(p) __p = (p);						\
	compiletime_assert_atomic_type(*p);				\
	kasan_check_read(__p, sizeof(*p));				\
	switch (sizeof(*p)) {						\
	case 1:								\
		asm volatile ("ldarb %w0, %1"				\
			: "=r" (*(__u8 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 2:								\
		asm volatile ("ldarh %w0, %1"				\
			: "=r" (*(__u16 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 4:								\
		asm volatile ("ldar %w0, %1"				\
			: "=r" (*(__u32 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	case 8:								\
		asm volatile ("ldar %0, %1"				\
			: "=r" (*(__u64 *)__u.__c)			\
			: "Q" (*__p) : "memory");			\
		break;							\
	}								\
	(typeof(*p))__u.__val;						\
})

#define smp_cond_load_relaxed(ptr, cond_expr)				\
({									\
	typeof(ptr) __PTR = (ptr);					\
	__unqual_scalar_typeof(*ptr) VAL;				\
	for (;;) {							\
		VAL = READ_ONCE(*__PTR);				\
		if (cond_expr)						\
			break;						\
		__cmpwait_relaxed(__PTR, VAL);				\
	}								\
	(typeof(*ptr))VAL;						\
})

#define smp_cond_load_acquire(ptr, cond_expr)				\
({									\
	typeof(ptr) __PTR = (ptr);					\
	__unqual_scalar_typeof(*ptr) VAL;				\
	for (;;) {							\
		VAL = smp_load_acquire(__PTR);				\
		if (cond_expr)						\
			break;						\
		__cmpwait_relaxed(__PTR, VAL);				\
	}								\
	(typeof(*ptr))VAL;						\
})

#define __smp_timewait_store(ptr, val)         \
               __cmpwait_relaxed(ptr, val)

/*
 * Redefine ARCH_TIMER_EVT_STREAM_PERIOD_US locally to avoid include hell.
 */
#define __ARCH_TIMER_EVT_STREAM_PERIOD_US 100UL
extern bool arch_timer_evtstrm_available(void);

/*
 * For coarse grained waits, allow overshoot by the event-stream period.
 * Defined without reference to ARCH_TIMER_EVT_STREAM_PERIOD_US to avoid
 * include hell.
 */
#define        SMP_TIMEWAIT_SLACK_COARSE_US    __ARCH_TIMER_EVT_STREAM_PERIOD_US

#define SMP_TIMEWAIT_SPIN_BASE         16
#define SMP_TIMEWAIT_CHECK_US          2UL

static inline u64 ___cond_timewait(u64 now, u64 prev, u64 end,
                                     u32 *spin, bool *wait, u64 slack)
{
       bool wfet = alternative_has_cap_unlikely(ARM64_HAS_WFXT);
       bool wfe, ev = arch_timer_evtstrm_available();
       u64 evt_period = __ARCH_TIMER_EVT_STREAM_PERIOD_US;
       u64 remaining = end - now;

       if (now >= end)
               return 0;

       if (slack == 0)
               slack = max(remaining, SMP_TIMEWAIT_CHECK_US);

       /*
        * Use WFE if there's enough slack to get an event-stream wakeup even
        * if we don't come out of the WFE due to natural causes.
        */
       wfe = ev && ((remaining + slack) > evt_period);

       if (wfe || wfet) {
               *wait = true;
               *spin = 0;
               return now;
       }

       /*
        * Our wait period is shorter than our best granularity. Spin.
        *
        * A time-check is expensive but not too expensive. Scale the
        * spin-count so we stay close to the fine-grained slack period.
        */
       *wait = false;
       if ((now - prev) < SMP_TIMEWAIT_CHECK_US)
               *spin <<= 1;
       else
               *spin = max((*spin >> 1) + (*spin >> 2), SMP_TIMEWAIT_SPIN_BASE);
       return now;
}

/*
 * Fine wait_policy: minimize the timeout delay while balancing against the
 * time spent in the WFE wait state.
 *
 * The worst case timeout delay is ARCH_TIMER_EVT_STREAM_PERIOD_US/2, which
 * would also be the worst case spin period.
 */
#define __smp_cond_timewait_fine(now, prev, end, spin, wait)           \
       __smp_cond_timewait(now, prev, end, spin, wait,                 \
                           0)
/*
 * Coarse wait_policy: minimizes the duration spent spinning at the cost of
 * potentially spending the available slack in a WFE wait state.
 *
 * The resultant worst case timeout delay is SMP_TIMEWAIT_SLACK_COARSE_US
 * (same as ARCH_TIMER_EVT_STREAM_PERIOD_US) and a spin period of no more
 * than SMP_TIMEWAIT_CHECK_US.
 */
#define __smp_cond_timewait_coarse(now, prev, end, spin, wait)         \
       ___cond_timewait(now, prev, end, spin, wait,                    \
                           SMP_TIMEWAIT_SLACK_COARSE_US)

#include <asm-generic/barrier.h>

#endif	/* __ASSEMBLY__ */

#endif	/* __ASM_BARRIER_H */
