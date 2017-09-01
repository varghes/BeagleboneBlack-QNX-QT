/*
 * $QNXLicenseC:
 * Copyright 2009, QNX Software Systems. 
 * 
 * Licensed under the Apache License, Version 2.0 (the "License"). You 
 * may not reproduce, modify or distribute this software except in 
 * compliance with the License. You may obtain a copy of the License 
 * at: http://www.apache.org/licenses/LICENSE-2.0 
 * 
 * Unless required by applicable law or agreed to in writing, software 
 * distributed under the License is distributed on an "AS IS" basis, 
 * WITHOUT WARRANTIES OF ANY KIND, either express or implied.
 *
 * This file may contain contributions from others, either as 
 * contributors under the License or as licensors under other terms.  
 * Please review this entire file for other proprietary rights or license 
 * notices, as well as the QNX Development Suite License Guide at 
 * http://licensing.qnx.com/license-guide/ for other information.
 * $
 */

 

#include "callout.ah"
#include <arm/at91sam9xx.h>

/*
 * -----------------------------------------------------------------------
 * Routine to patch callout code for IRC
 *
 * On entry:
 *	r0 - physical address of syspage
 *	r1 - virtual  address of syspage
 *	r2 - offset from start of syspage to start of the callout routine
 *	r3 - offset from start of syspage to read/write data used by callout
 * -----------------------------------------------------------------------
 */
interrupt_patch_aic:
	stmdb	sp!,{r4,lr}
	add		r4, r0, r2					// address of callout routine

	/*
	 * Map interrupt controller registers
	 */
	mov		r0, #AT91_AIC_SIZE	// size of AIC registers
	ldr		r1, [sp, #8]
	ldr		r1, [r1]
	bl		callout_io_map

	/*
	 * Patch the callout routine
	 */
	CALLOUT_PATCH	r4, r0, r1, r2, ip
	ldmia	sp!,{r4,pc}
    

/*
 * -----------------------------------------------------------------------
 * Identify interrupt source.
 *
 * Returns interrupt number in r4
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_id_at91sam9xx_aic, 0, interrupt_patch_aic)
    /*
     * Get the IRC interrupt controller base address (patched)
     */
    mov		ip,     #0x000000ff
    orr		ip, ip, #0x0000ff00
    orr		ip, ip, #0x00ff0000
    orr		ip, ip, #0xff000000

    /*
     * Get current interrupt ID.
     * Auto-clears ICCR for returned interrupt.
     */ 
    ldr     r4, [ip, #AT91_AIC_IVR]

	/* Check for spurious interrupt */
	cmp     r4, #0
.ifdef	SYSPAGE_SPU_CNT
	bpl		1f
	/* Got spurious interrupt */
	/* r5 holds syspage ptr   */
	/* ++ syspage->spare[0]   */
	ldr		r0, [r5, #0x44]
	add		r0, r0, #1
	str		r0, [r5, #0x44]
.endif
    /*
     * Indicate we've completed processing of this spurious interrupt with an EOI.
	 * For EOI, the value written doesn't matter.
     */
    strmi   r0, [ip, #AT91_AIC_EOICR]
    bmi     2f

	/* Non-spurious interrupt */
1:
    /*
     *  Disable interrupt
     */
    mov    r0, #1
    mov    r0, r0, lsl r4
    str    r0, [ip,  #AT91_AIC_IDCR] 
2:
    CALLOUT_END(interrupt_id_at91sam9xx_aic)


/*
 * -----------------------------------------------------------------------
 * Acknowledge specified interrupt
 *
 * On entry:
 *	r4 contains the interrupt number
 *	r7 contains the interrupt mask count
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_eoi_at91sam9xx_aic, 0, interrupt_patch_aic)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

    /*
     * Indicate we've completed processing of interrupt.
	 * For EOI, the value written doesn't matter.
     */
    str     r0, [ip, #AT91_AIC_EOICR]

    /*
     * Only unmask interrupt if mask count is zero
     */
    teq     r7, #0
    
    /*
     * Now we need to unmask the source for the specified interrupt source
     */
    moveq  r0, #1
    moveq  r0, r0, lsl r4
    streq  r0, [ip,  #AT91_AIC_IECR] 

CALLOUT_END(interrupt_eoi_at91sam9xx_aic)


/*
 * -----------------------------------------------------------------------
 * Mask specified interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_mask_at91sam9xx_aic, 0, interrupt_patch_aic)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000

    /*
     * Mask the interrupt
     */
    mov    r0, #1
    mov    r0, r0, lsl r1
    str    r0, [ip,  #AT91_AIC_IDCR] 

    mov     r0, #0
    mov     pc, lr
CALLOUT_END(interrupt_mask_at91sam9xx_aic)


/*
 * -----------------------------------------------------------------------
 * Unmask specified interrupt
 *
 * On entry:
 *	r0 - syspage_ptr
 *	r1 - interrupt number
 *
 * Returns:
 *	r0 - error status
 * -----------------------------------------------------------------------
 */
CALLOUT_START(interrupt_unmask_at91sam9xx_aic, 0, interrupt_patch_aic)
	/*
	 * Get the interrupt controller base address (patched)
	 */
	mov		ip,     #0x000000ff
	orr		ip, ip, #0x0000ff00
	orr		ip, ip, #0x00ff0000
	orr		ip, ip, #0xff000000
    /*
     * We don't allow to enable FIQ interrupt at source 0
     */
    teq     r1, #0
    
    /*
     * Unmask the interrupt
     */
    movne   r0, #1
    movne   r0, r0, lsl r1
    strne   r0, [ip,  #AT91_AIC_IECR] 

	mov		r0, #0
	mov		pc, lr
CALLOUT_END(interrupt_unmask_at91sam9xx_aic)

