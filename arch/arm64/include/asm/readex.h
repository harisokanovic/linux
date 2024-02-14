/* SPDX-License-Identifier: GPL-2.0 */
/*
 * Based on arch/arm64/include/asm/rwonce.h
 *
 * Copyright (C) 2020 Google LLC.
 * Copyright (C) 2024 Amazon.com, Inc. or its affiliates.
 */

#ifndef __ASM_READEX_H
#define __ASM_READEX_H

#define __LOAD_EX(sfx, regs...) "ldaxr" #sfx "\t" #regs

#define __READ_ONCE_EX(x)						\
({									\
	typeof(&(x)) __x = &(x);					\
	int atomic = 1;							\
	union { __unqual_scalar_typeof(*__x) __val; char __c[1]; } __u;	\
	switch (sizeof(x)) {						\
	case 1:								\
		asm volatile(__LOAD_EX(b, %w0, %1)			\
			: "=r" (*(__u8 *)__u.__c)			\
			: "Q" (*__x) : "memory");			\
		break;							\
	case 2:								\
		asm volatile(__LOAD_EX(h, %w0, %1)			\
			: "=r" (*(__u16 *)__u.__c)			\
			: "Q" (*__x) : "memory");			\
		break;							\
	case 4:								\
		asm volatile(__LOAD_EX(, %w0, %1)			\
			: "=r" (*(__u32 *)__u.__c)			\
			: "Q" (*__x) : "memory");			\
		break;							\
	case 8:								\
		asm volatile(__LOAD_EX(, %0, %1)			\
			: "=r" (*(__u64 *)__u.__c)			\
			: "Q" (*__x) : "memory");			\
		break;							\
	default:							\
		atomic = 0;						\
	}								\
	atomic ? (typeof(*__x))__u.__val : (*(volatile typeof(__x))__x);\
})

#endif
