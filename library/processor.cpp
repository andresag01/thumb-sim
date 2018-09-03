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
#include "simulator/processor.h"

#include "simulator/debug.h"
#include "simulator/decode.h"
#include "simulator/execute.h"
#include "simulator/fetch.h"
#include "simulator/memory.h"
#include "simulator/regfile.h"
#include "simulator/stats.h"
#include "simulator/utils.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>

Processor::Processor(uint32_t memSizeWordsIn, uint32_t memAccessWidthWordsIn)
{
    stats = new Statistics();
    regFile = new RegFile();
    mem = new Memory(memSizeWordsIn, memAccessWidthWordsIn, 2);
    fetch = new Fetch(mem, regFile, stats);
    decode = new Decode(fetch, regFile);
    execute = new Execute(fetch, decode, regFile, mem, stats);

    /* Add system configuration statistics */
    stats->setMemSizeWords(mem->getMemSizeWords());
    stats->setMemAccessWidthWords(mem->getMemAccessWidthWords());
}

Processor::~Processor()
{
    delete stats;
    delete regFile;
    delete mem;
    delete fetch;
    delete decode;
    delete execute;
}

int Processor::simulateCycle()
{
    stats->addCycle();

    execute->run();
    decode->run();
    fetch->run();

    mem->run();

    DEBUG_CMD(DEBUG_REGFILE, regFile->print());

    return 0;
}

int Processor::reset(char *programBinFile)
{
    int ret;
    uint32_t pcAddr;
    uint32_t programByteSize;
    uint32_t sp;

    /* Load the program binary in memory */
    ret = mem->loadProgram(programBinFile, pcAddr, programByteSize);
    if (ret != 0)
    {
        fprintf(stderr, "Failed to load program binary in memory\n");
        exit(1);
    }
    else if ((pcAddr & 1) == 0)
    {
        fprintf(stderr,
                "Reset vector contains an ARM address 0x%08" PRIX32 "\n",
                pcAddr);
        exit(1);
    }

    regFile->write(Reg::PC, pcAddr & ~1);

    DEBUG_CMD(DEBUG_MEMORY,
              printf("Program size is %" PRIu32 " bytes (%" PRIu32 " words)\n",
                     programByteSize,
                     BYTE_TO_WORD_SIZE(programByteSize)));

    /* Load the stack pointer from the first entry in the vector table */
    mem->loadWord(RESET_VECTOR_SP_ADDRESS, sp);
    regFile->write(regFile->getActiveSp(), sp);

    /*
     * Give fetch a pointer to the execute stage so that it can work out when
     * the pipeline is not stalled and avoid unecessary memory requests
     */
    fetch->setExecute(execute);

    DEBUG_CMD(DEBUG_MEMORY, mem->dump());

    /* Record the program size statistics (not including the header) */
    stats->setProgramSizeBytes(programByteSize);

    return 0;
}
