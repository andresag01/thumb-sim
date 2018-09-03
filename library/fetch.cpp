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
#include "simulator/fetch.h"

#include "simulator/debug.h"
#include "simulator/execute.h"
#include "simulator/memory.h"
#include "simulator/regfile.h"
#include "simulator/utils.h"

#include <cinttypes>
#include <cstdint>
#include <cstring>

Fetch::Fetch(Memory *memIn, RegFile *regFileIn, Statistics *statsIn) :
    mem(memIn),
    regFile(regFileIn),
    stats(statsIn)
{
    instBuffer = new uint16_t[memIn->getMemAccessWidthWords() * 2];
}

Fetch::~Fetch()
{
    delete instBuffer;
}

void Fetch::flush()
{
    flushPending = true;
}

void Fetch::setExecute(Execute *executeIn)
{
    execute = executeIn;
}

void Fetch::print()
{
    uint32_t i;
    std::string prefix = "    ";

    printf("Fetch: base:0x%08" PRIX32 " valid:%s flushPending:%s "
           "issuedMemAccess:%s memToken:0x%08" PRIX32 "\n",
           instBufferBaseAddr,
           BOOL_TO_STR(instBufferValid),
           BOOL_TO_STR(flushPending),
           BOOL_TO_STR(issuedMemAccess),
           memToken);

    for (i = 0; i < mem->getMemAccessWidthWords() * 2; i++)
    {
        printf("%s0x%08" PRIX32 ": %04" PRIX16 "\n",
               prefix.c_str(),
               i * THUMB_INST_BYTES + instBufferBaseAddr,
               instBuffer[i]);
    }
}

int Fetch::getNextInst(uint16_t &inst)
{
    uint32_t pc;

    regFile->read(Reg::PC, pc);

    if (!instBufferValid || flushPending)
    {
        return -1;
    }
    else if (mem->getMemAccessWidthBaseByteAddr(pc) != instBufferBaseAddr)
    {
        fprintf(stderr,
                "Unpredictable state: instruction buffer (0x%08" PRIX32
                ") is valid and out of sync with pc (0x%08" PRIX32 ")\n",
                instBufferBaseAddr,
                mem->getMemAccessWidthBaseByteAddr(pc));
        exit(1);
    }

    /* Get the next instruction to decode */
    inst = instBuffer[mem->getMemAccessWidthInstOffset(pc)];

    /* Move the pc to the next instruction */
    regFile->write(Reg::PC, NEXT_THUMB_INST(pc));

    return 0;
}

int Fetch::run()
{
    int ret;
    uint32_t pc;

    if (flushPending)
    {
        /* Flushing the instruction buffer */
        instBufferValid = false;
        instBufferBaseAddr = 0xFFFFFFFF;
        memset(instBuffer,
               0x0,
               sizeof(uint16_t) * mem->getMemAccessWidthWords() * 2);

        /* We no longer care about previously issued fetches */
        issuedMemAccess = false;

        flushPending = false;

        DEBUG_CMD(DEBUG_FETCH, printf("Fetch: flushing\n"));
    }

    /* Load the pc and  tuple metadata */
    regFile->read(Reg::PC, pc);

    if (issuedMemAccess)
    {
        /* Check whether the data was loaded */
        ret = mem->retrieveWideLoad(memToken,
                                    reinterpret_cast<uint32_t *>(instBuffer));
        if (ret == 0)
        {
            instBufferValid = true;
            instBufferBaseAddr = mem->getMemAccessWidthBaseByteAddr(pc);
            issuedMemAccess = false;

            DEBUG_CMD(DEBUG_FETCH, print());
        }
        else
        {
            DEBUG_CMD(DEBUG_FETCH, printf("Fetch: memory reply not ready\n"));
        }
    }

    /*
     * We want to start a fetch whenever:
     *  - The instruction buffer is invalid
     *  - The instruction buffer is valid, but the pc is not contained there
     * Since fetches take two cycles, we need to start the fetch operation
     * early, so we make the pc point to the next instruction instead of the
     * one currently being decoded
     */
    if (instBufferValid)
    {
        /*
         * Fetch operations take at least two cycles:
         *  1. Place the memory request
         *  2. Retrieve the memory response and store it in the instruction
         *     buffer
         * When the instruction buffer is valid, and the pipeline is operating
         * normally, we need to start the fetch operation when the pc is
         * pointing to the last instruction in the instruction buffer and
         * the execution unit is not currently stalled.
         *
         * Note that if we were to fetch when the execution unit is stalled,
         * then we would have to discard the fetched data, meaning that there
         * could potentially be several unecessary memory requests.
         *
         * Note that this trick only works if the instruction buffer is at
         * least one word in length, otherwise the pc base address will be
         * different when we get the memory response
         */
        pc = NEXT_THUMB_INST(pc);
    }

    if (!instBufferValid ||
        (!execute->isStalled() &&
         mem->getMemAccessWidthBaseByteAddr(pc) != instBufferBaseAddr))
    {
        /* In this case, we need to start a fetch from memory */

        /* Issue a load request */
        ret = mem->requestLoad(Component::FETCH, pc, memToken);
        if (ret == 0)
        {
            /* Memory device busy, cannot issue fetch request */
            issuedMemAccess = true;

            DEBUG_CMD(DEBUG_FETCH,
                      printf("Fetch: requested from pc %08" PRIX32 "\n", pc));
        }
        else
        {
            DEBUG_CMD(DEBUG_FETCH,
                      printf("Fetch: Could not create memory request\n"));
        }

        /* Record that this fetch required a memory access */
        stats->addFetchCycle();
    }
    else
    {
        DEBUG_CMD(DEBUG_FETCH, printf("Fetch: stalled\n"));
    }

    return 0;
}
