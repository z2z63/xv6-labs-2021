	.text

	/*
         * save the old thread's registers,
         * restore the new thread's registers.
         */

	.globl thread_switch
thread_switch:
	/* YOUR CODE HERE */
	// 将寄存器保存到old中，将new保存的值加载到寄存器
	/* void thread_switch(uint64 old, uint64 new); */
	// 保存寄存器
	sd sp, 0(a0)
	sd ra, -8(sp)

	// callee saved
	sd s0, -16(sp)
	sd s1, -24(sp)
	sd s2, -32(sp)
	sd s3, -40(sp)
	sd s4, -48(sp)
	sd s5, -56(sp)
	sd s6, -64(sp)
	sd s7, -72(sp)
	sd s8, -80(sp)
	sd s9, -88(sp)
    sd s10, -96(sp)
    sd s11, -104(sp)

    ld sp, 0(a1)
    ld ra, -8(sp)

    // callee saved
    ld s0, -16(sp)
    ld s1, -24(sp)
    ld s2, -32(sp)
    ld s3, -40(sp)
    ld s4, -48(sp)
    ld s5, -56(sp)
    ld s6, -64(sp)
    ld s7, -72(sp)
    ld s8, -80(sp)
    ld s9, -88(sp)
    ld s10, -96(sp)
    ld s11, -104(sp)


	ret    /* return to ra */
