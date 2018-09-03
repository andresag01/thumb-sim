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
#ifndef _PROCESSOR_H_
#define _PROCESSOR_H_

#include "simulator/decode.h"
#include "simulator/execute.h"
#include "simulator/fetch.h"
#include "simulator/memory.h"
#include "simulator/regfile.h"
#include "simulator/stats.h"

#include <cstdint>

class Processor
{
public:
    Processor(uint32_t memSizeWordsIn, uint32_t memAccessWidthWordsIn);
    ~Processor();

    int simulateCycle();
    int reset(char *programBinFile);

private:
    Statistics *stats;
    RegFile *regFile;
    Memory *mem;
    Fetch *fetch;
    Decode *decode;
    Execute *execute;
};

#endif /* _PROCESSOR_H_ */
