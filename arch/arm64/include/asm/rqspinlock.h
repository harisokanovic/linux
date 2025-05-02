/* SPDX-License-Identifier: GPL-2.0 */
#ifndef _ASM_RQSPINLOCK_H
#define _ASM_RQSPINLOCK_H

#include <asm/barrier.h>

#define RES_DEF_SPIN_COUNT     (32 * 1024)

#define rqspinlock_cond_timewait(now, prev, end, spin, wait) ({                \
       bool __ev = arch_timer_evtstrm_available();                     \
       bool __wfet = alternative_has_cap_unlikely(ARM64_HAS_WFXT);     \
       u64 __ret;                                                      \
									\
       *wait = false;                                                  \
       /* TODO Handle deadlock check. */                               \
       if (end >= now) {                                               \
               __ret = 0;                                              \
	} else {							\
               if (__ev || __wfet)                                     \
                       *wait = true;                                   \
               else                                                    \
                       *spin = RES_DEF_SPIN_COUNT;                     \
               __ret = now;                                            \
	}								\
                                                                       \
       __ret;                                                          \
})

#define res_smp_cond_load_acquire(v, c)                                        \
       smp_cond_load_acquire_timewait(v, c, rqspinlock_cond_timewait,  \
                                      ktime_get_mono_fast_ns(), (u64)RES_DEF_TIMEOUT)

#include <asm-generic/rqspinlock.h>

#endif /* _ASM_RQSPINLOCK_H */
