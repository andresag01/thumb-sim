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
#ifndef _UTILS_H_
#define _UTILS_H_

#include "simulator/config.h"

#define BYTES_PER_WORD 4
#define BITS_PER_BYTE 8
#define BITS_PER_WORD (BYTES_PER_WORD * BITS_PER_BYTE)
#define BITS_PER_HALFWORD 16

#define GET_BYTE_INDEX(addr) ((addr) & (BYTES_PER_WORD - 1))
#define GET_WORD_ADDRESS(addr) ((addr) & ~(BYTES_PER_WORD - 1))
#define GET_WORD_INDEX(addr) (GET_WORD_ADDRESS(addr) >> 2)

#define GET_BIT_AT_POS(val, x) (((val) >> (x)) & 0x1)
#define SET_BIT_AT_POS(val, x, y) (((val) & ~(0x1 << x)) | (((y)&0x1) << x))

#define THUMB_INST_BYTES 2

#define NEXT_THUMB_INST(addr) ((addr) + THUMB_INST_BYTES)
#define PREV_THUMB_INST(addr) ((addr)-THUMB_INST_BYTES)

#define RESET_VECTOR_SP_ADDRESS 0x00000000
#define RESET_VECTOR_PC_ADDRESS 0x00000004

#define BITS_PER_HANDLE 16
#define BITS_PER_OFFSET (BITS_PER_WORD - BITS_PER_HANDLE)

#define ALIGN(addr, align) ((addr) & ~(align - 1))

#define BYTE_TO_WORD_SIZE(wsize) \
    ((wsize) / BYTES_PER_WORD + (((wsize) % BYTES_PER_WORD != 0) ? 1 : 0))
#define WORD_TO_BYTE_SIZE(bsize) ((bsize)*BYTES_PER_WORD)

#define BOOL_TO_STR(val) ((val) ? "true" : "false")

#endif /* _UTILS_H_ */
