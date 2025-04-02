	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
    addi sp, sp, -8
    sd   ra, 0(sp)
    sd   sp, 0(a0)
    ld   sp, 0(a1)
    ld   ra, 0(sp)
    addi sp, sp, 8
	ret    /* return to ra */
