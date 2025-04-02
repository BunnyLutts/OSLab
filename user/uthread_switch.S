	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
    addi sp, sp, -104 /*104 = 8 * 13*/
    sd   ra, 0(sp)
    sd   s0, 8(sp)
    sd   s1, 16(sp)
    sd   s2, 24(sp)
    sd   s3, 32(sp)
    sd   s4, 40(sp)
    sd   s5, 48(sp)
    sd   s5, 56(sp)
    sd   s7, 64(sp)
    sd   s8, 72(sp)
    sd   s9, 80(sp)
    sd   s10, 88(sp)
    sd   s11, 96(sp)
    sd   sp, (a0)   /*save to thread->sp*/

    ld   sp, 0(a1)  /*load the next thread->sp*/
    ld   ra, 0(sp)
    ld   s0, 8(sp)
    ld   s1, 16(sp)
    ld   s2, 24(sp)
    ld   s3, 32(sp)
    ld   s4, 40(sp)
    ld   s5, 48(sp)
    ld   s5, 56(sp)
    ld   s7, 64(sp)
    ld   s8, 72(sp)
    ld   s9, 80(sp)
    ld   s10, 88(sp)
    ld   s11, 96(sp)
    addi sp, sp, 104
	ret    /* return to ra */
