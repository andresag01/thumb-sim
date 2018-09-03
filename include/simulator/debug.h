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
#ifndef _DEBUG_H_
#define _DEBUG_H_

#define DEBUG_FETCH 0x00000001
#define DEBUG_DECODE 0x00000002
#define DEBUG_MEMORY 0x00000004
#define DEBUG_REGFILE 0x00000010
#define DEBUG_EXECUTE 0x00000020
#define DEBUG_ALL 0xFFFFFFFF

#define DEBUG_MASK (0xFFFFFFFF)

#if defined(DEBUG_ENABLED)
#define DEBUG_CMD(flags, x)            \
    if (((flags) & (DEBUG_MASK)) != 0) \
    {                                  \
        x;                             \
        fflush(stdout);                \
    }
#else
#define DEBUG_CMD(flags, x)
#endif /* DEBUG_ENABLED */

#endif /* _DEBUG_H_ */
