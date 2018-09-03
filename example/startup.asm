/*
 * MIT License
 *
 * Copyright (c) 2018 Andres Amaya Garcia
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */
    .text
    .thumb
    .cpu cortex-m0

.word   _stack_end  /* stack top address */
.word   _start      /* 1 Reset */
.word   die         /* 2 NMI */
.word   die         /* 3 HardFault */
.word   die         /* 4 MemManage */
.word   die         /* 5 BusFault */
.word   die         /* 6 UsageFault */
.word   die         /* 7 RESERVED */
.word   die         /* 8 RESERVED */
.word   die         /* 9 RESERVED*/
.word   die         /* 10 RESERVED */
.word   die         /* 11 SVCall */
.word   die         /* 12 Debug Monitor */
.word   die         /* 13 RESERVED */
.word   die         /* 14 PendSV */
.word   die         /* 15 SysTick */
.word   die         /* 16 External Interrupt(0) */
.word   die         /* 17 External Interrupt(1) */
.word   die         /* 18 External Interrupt(2) */
.word   die         /* 19 ...   */


    .thumb_func
die:
    b .

    .thumb_func
        .global putchar
putchar:
    /*
     * The simulator has repurposed the CPS thumb instruction to print to
     * STDOUT the character in r0
     */
    cpsid i
    bx lr

    .thumb_func
        .global fail
fail:
    svc 10

    .thumb_func
        .global success
success:
    bkpt 0

    .thumb_func
        .global _start
_start:
    /* Execute the benchmark */
    bl main
    bl success

.end
