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
#include "simulator/execute.h"

#include "simulator/debug.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

int Execute::bkpt(uint32_t im)
{
    delete decodedInst;
    decodedInst = NULL;

    DEBUG_CMD(DEBUG_MEMORY, mem->dump());

    stats->print();
    printf("Hit breakpoint with value %" PRIu32 ". Terminating...\n", im);
    exit(im);
}

int Execute::nop()
{
    /* Do nothing ... */

    /* Record the instruction stats */
    stats->addInstruction(Instruction::NOP);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" NOP\n"));
    return 0;
}

int Execute::svc(uint32_t im)
{
    delete decodedInst;
    decodedInst = NULL;

    fprintf(stderr, "Reached SVC (im %" PRIu32 ") instruction\n", im);
    DEBUG_CMD(DEBUG_MEMORY, mem->dump());
    exit(im);
}

/* Repurpose this instruction for printing a character in register r0 */
int Execute::cps(uint32_t drm)
{
    char c = static_cast<char>(drm & 0xFF);

    putchar(c);
    fflush(stdout);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" CPS\n"));
    return 0;
}
