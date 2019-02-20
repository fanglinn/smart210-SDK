/*
 * Copyright (C) 2009 Samsung Electronics
 * Kyungmin Park <kyungmin.park@samsung.com>
 * Minkyu Kang <mk7.kang@samsung.com>
 *
 * SPDX-License-Identifier:	GPL-2.0+
 */

#include <config.h>
#include <version.h>
#include <asm/arch/cpu.h>
#include <asm/arch/power.h>

/*
 * Register usages:
 *
 * r5 has zero always
 */

	.globl lowlevel_init
lowlevel_init:
	mov	r9, lr

#ifdef CONFIG_SPL_BUILD
        bl clock_init                   /* clock init */
        bl ddr_init                     /* DDR init */

        /* add by Flinn, for uart */
        ldr r0, =0xE0200000     /* GPA0_CON */
        ldr     r1, =0x22222222
        str     r1, [r0]

#endif
        mov     pc, r9                /* return */
