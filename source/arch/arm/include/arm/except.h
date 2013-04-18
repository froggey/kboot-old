/*
 * Copyright (C) 2013 Alex Smith
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/**
 * @file
 * @brief		ARM exception handling definitions.
 */

#ifndef __ARM_EXCEPT_H
#define __ARM_EXCEPT_H

#include <types.h>

/** Structure defining an interrupt stack frame. */
typedef struct interrupt_frame {
	unsigned long sp;			/**< SP/R13 from previous mode. */
	unsigned long lr;			/**< LR/R14 from previous mode. */
	unsigned long r0;			/**< R0. */
	unsigned long r1;			/**< R1. */
	unsigned long r2;			/**< R2. */
	unsigned long r3;			/**< R3. */
	unsigned long r4;			/**< R4. */
	unsigned long r5;			/**< R5. */
	unsigned long r6;			/**< R6. */
	unsigned long r7;			/**< R7. */
	unsigned long r8;			/**< R8. */
	unsigned long r9;			/**< R9. */
	unsigned long r10;			/**< R10. */
	unsigned long r11;			/**< R11. */
	unsigned long r12;			/**< R12. */
	unsigned long pc;			/**< PC/R15 from previous mode. */
	unsigned long spsr;			/**< Saved Program Status Register. */
} __packed interrupt_frame_t;

/** Exception vector table indexes. */
#define ARM_VECTOR_COUNT		8	/**< Number of exception vectors. */
#define ARM_VECTOR_RESET		0	/**< Reset. */
#define ARM_VECTOR_UNDEFINED		1	/**< Undefined Instruction. */
#define ARM_VECTOR_SYSCALL		2	/**< Supervisor Call. */
#define ARM_VECTOR_PREFETCH_ABORT	3	/**< Prefetch Abort. */
#define ARM_VECTOR_DATA_ABORT		4	/**< Data Abort. */
#define ARM_VECTOR_IRQ			6	/**< IRQ. */
#define ARM_VECTOR_FIQ			7	/**< FIQ. */

extern void arm_undefined(void);
extern void arm_prefetch_abort(void);
extern void arm_data_abort(void);

extern void arm_undefined_handler(interrupt_frame_t *frame);
extern void arm_prefetch_abort_handler(interrupt_frame_t *frame);
extern void arm_data_abort_handler(interrupt_frame_t *frame);

#endif /* __ARM_EXCEPT_H */
