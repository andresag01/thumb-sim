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
#ifndef _FETCH_H_
#define _FETCH_H_

#include "simulator/memory.h"
#include "simulator/regfile.h"
#include "simulator/stats.h"

/* Forward definition to avoid circular inclusion problem */
class Execute;

class Fetch
{
public:
    Fetch(Memory *memIn, RegFile *regFileIn, Statistics *statsIn);
    ~Fetch();

    void flush();
    int getNextInst(uint16_t &inst);

    int run();

    void setExecute(Execute *executeIn);

    void print();

private:
    Memory *mem;
    RegFile *regFile;
    Execute *execute;
    Statistics *stats;

    uint32_t memToken{ 0xFFFFFFFF };
    bool issuedMemAccess{ false };
    uint16_t *instBuffer{ nullptr };
    uint32_t instBufferBaseAddr{ 0 };
    bool instBufferValid{ false };
    bool flushPending{ false };
};

#endif /* _FETCH_H_ */
