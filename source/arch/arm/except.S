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
 * @brief		ARM exception handlers.
 */

#include <arch/page.h>

#include <arm/asm.h>
#include <arm/cpu.h>

.section .text, "ax", %progbits

/** Switch into supervisor mode and create an interrupt frame on the stack.
 * @param offset	Exception-dependent offset of the LR value. */
.macro EXCEPTION_ENTER offset
	/* Subtract the exception-dependent offset from LR so that we return
	 * to the correct place. */
	sub	lr, lr, #\offset

	/* Save LR and SPSR to the supervisor mode stack. */
	srsdb	sp!, #ARM_MODE_SVC

	/* Switch to supervisor mode, with IRQs and FIQs disabled. */
	cpsid	if, #ARM_MODE_SVC

	/* Save remaining registers. */
	stmfd	sp!, { r0-r12 }

	/* Save SP and LR. */
	add	r0, sp, #60
	stmfd 	sp!, { r0, lr }

	/* Set R0 to the current SP as the argument to the handler. */
	mov	r0, sp
.endm

/** Undefined Instruction exception handler. */
FUNCTION_START(arm_undefined)
	EXCEPTION_ENTER 4
	bl	arm_undefined_handler
FUNCTION_END(arm_undefined)

/** Prefetch Abort exception handler. */
FUNCTION_START(arm_prefetch_abort)
	EXCEPTION_ENTER 4
	bl	arm_prefetch_abort_handler
FUNCTION_END(arm_prefetch_abort)

/** Data Abort exception handler. */
FUNCTION_START(arm_data_abort)
	EXCEPTION_ENTER 8
	bl	arm_data_abort_handler
FUNCTION_END(arm_data_abort)
