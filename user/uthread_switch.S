	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
	/* save callee-save registers of the old thread */
	sd ra, 0(a0)	/*Save return address*/
	sd sp, 8(a0)    /* Save s0 registers*/
	sd s0, 16(a0)
	sd s1, 24(a0)    /* Save s0 registers*/
	sd s2, 32(a0)    /* Save s0 registers*/
	sd s3, 40(a0)    /* Save s0 registers*/
	sd s4, 48(a0)    /* Save s0 registers*/
	sd s5, 56(a0)    /* Save s0 registers*/
	sd s6, 64(a0)    /* Save s0 registers*/
	sd s7, 72(a0)    /* Save s0 registers*/
	sd s8, 80(a0)    /* Save s0 registers*/
	sd s9, 88(a0)    /* Save s0 registers*/
	sd s10, 96(a0)    /* Save s0 registers*/
	sd s11, 104(a0)    /* Save s0 registers*/
	
	/* load callee-save registers of the new thread */
	ld ra, 0(a1)	 /* restore return address*/
	ld sp, 8(a1)
	ld s0, 16(a1)     /* restore s0 registers*/
	ld s1, 24(a1)    /* restore s0 registers*/
	ld s2, 32(a1)    /* restore s0 registers*/
	ld s3, 40(a1)    /* restore s0 registers*/
	ld s4, 48(a1)    /* restore s0 registers*/
	ld s5, 56(a1)    /* restore s0 registers*/
	ld s6, 64(a1)    /* restore s0 registers*/
	ld s7, 72(a1)    /* restore s0 registers*/
	ld s8, 80(a1)    /* restore s0 registers*/
	ld s9, 88(a1)    /* restore s0 registers*/
	ld s10, 96(a1)   /* restore s0 registers*/
	ld s11, 104(a1)  /* restore s0 registers*/

	/* return to ra */
	ret    
