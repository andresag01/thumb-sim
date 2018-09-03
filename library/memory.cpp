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
#include "simulator/memory.h"

#include "simulator/debug.h"
#include "simulator/utils.h"

#include <cinttypes>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <string>

Memory::Memory(uint32_t memSizeWordsIn,
               uint32_t memAccessWidthWordsIn,
               uint32_t pipelineSizeIn)
{
    uint32_t i;

    memAccessWidthWords = memAccessWidthWordsIn;
    memSizeWords = memSizeWordsIn;
    if (memSizeWordsIn % memAccessWidthWords > 0)
    {
        memSizeWords = memSizeWords + memAccessWidthWords -
            (memSizeWordsIn % memAccessWidthWords);
    }
    mem = new uint32_t[memSizeWords];

    for (i = 0; i < memSizeWords; i++)
    {
        mem[i] = 0;
    }

    /* The +1 is to not clear served responses early */
    pipeline = new MemoryRequest[pipelineSizeIn + 1];
    pipelineSize = pipelineSizeIn + 1;
    nextReqIndex = 0;
    nextToken = 0;

    for (i = 0; i < pipelineSize; i++)
    {
        pipeline[i].issuer = Component::NONE;
        pipeline[i].type = MemoryAccessType::NONE;
        pipeline[i].token = 0;
        pipeline[i].byteAddr = 0;
        pipeline[i].reqData = new uint32_t[memAccessWidthWords];
        pipeline[i].reqEnable = new bool[memAccessWidthWords];
        pipeline[i].respData = new uint32_t[memAccessWidthWords];
    }
}

Memory::~Memory()
{
    uint32_t i;

    delete mem;

    for (i = 0; i < pipelineSize; i++)
    {
        delete pipeline[i].reqData;
        delete pipeline[i].reqEnable;
        delete pipeline[i].respData;
    }
    delete pipeline;
}

int Memory::loadProgram(char *programFile,
                        uint32_t &pc,
                        uint32_t &programByteSize)
{
    int ret = -1;
    uint32_t addr = 0x00000000;
    std::ifstream inBin{ programFile,
                         std::ios::in | std::ios::binary | std::ios::ate };
    std::streampos binSize;

    if (!inBin.is_open())
    {
        fprintf(stderr, "Could not open '%s'\n", programFile);
        return ret;
    }

    binSize = inBin.tellg();
    if (binSize >= WORD_TO_BYTE_SIZE(memSizeWords))
    {
        std::cerr << "Program binary is too large for memory (" << binSize
                  << " bytes, " << BYTE_TO_WORD_SIZE(binSize) << " words)"
                  << std::endl;
        goto exit;
    }

    /* Allocate space for the program binary */
    inBin.seekg(0, std::ios::beg);
    inBin.read(reinterpret_cast<char *>(mem + GET_WORD_INDEX(addr)), binSize);
    if (binSize != inBin.gcount())
    {
        fprintf(stderr, "Failed to read full program binary\n");
        goto exit;
    }

    programByteSize = binSize;

    /*
     * Load the program counter from the second entry in the vector table into
     * PC.
     */
    pc = mem[GET_WORD_INDEX(addr + RESET_VECTOR_PC_ADDRESS)];

    ret = 0;

exit:
    inBin.close();

    return ret;
}

int Memory::requestLoad(Component issuer, uint32_t byteAddr, uint32_t &token)
{
    if (pipeline[nextReqIndex].issuer != Component::NONE)
    {
        /* Cannot place more than one requests in the same cycle */
        return -1;
    }

    pipeline[nextReqIndex].issuer = issuer;
    pipeline[nextReqIndex].type = MemoryAccessType::LOAD;
    pipeline[nextReqIndex].token = nextToken;
    pipeline[nextReqIndex].byteAddr = byteAddr;

    token = nextToken++;

    return 0;
}

bool Memory::isAvailable()
{
    return pipeline[nextReqIndex].issuer == Component::NONE;
}

int Memory::requestStore(Component issuer,
                         uint32_t byteAddr,
                         uint32_t data,
                         uint32_t &token)
{
    if (pipeline[nextReqIndex].issuer != Component::NONE)
    {
        /* Cannot place more than one requests in the same cycle */
        return -1;
    }

    pipeline[nextReqIndex].issuer = issuer;
    pipeline[nextReqIndex].type = MemoryAccessType::STORE;
    pipeline[nextReqIndex].token = nextToken;
    pipeline[nextReqIndex].byteAddr = byteAddr;
    pipeline[nextReqIndex].reqData[0] = data;

    token = nextToken++;

    return 0;
}

int Memory::retrieveLoad(uint32_t token, uint32_t &data)
{
    uint32_t prevRespIndex =
        (nextReqIndex == 0) ? pipelineSize - 1 : nextReqIndex - 1;
    uint32_t wordIndex;

    if (pipeline[prevRespIndex].token == token)
    {
        wordIndex =
            getMemAccessWidthWordIndex(pipeline[prevRespIndex].byteAddr);
        data = pipeline[prevRespIndex].respData[wordIndex];
        return 0;
    }
    else
    {
        return -1;
    }
}

int Memory::retrieveStore(uint32_t token)
{
    uint32_t prevRespIndex =
        (nextReqIndex == 0) ? pipelineSize - 1 : nextReqIndex - 1;

    if (pipeline[prevRespIndex].token == token)
    {
        return 0;
    }
    else
    {
        return -1;
    }
}

int Memory::retrieveWideLoad(uint32_t token, uint32_t *data)
{
    uint32_t prevRespIndex =
        (nextReqIndex == 0) ? pipelineSize - 1 : nextReqIndex - 1;

    if (pipeline[prevRespIndex].token == token)
    {
        if (data != nullptr)
        {
            memcpy(data,
                   pipeline[prevRespIndex].respData,
                   memAccessWidthWords * sizeof(uint32_t));
        }
        return 0;
    }
    else
    {
        return -1;
    }
}

int Memory::run()
{
    uint32_t wordBaseAddr;
    uint32_t nextRespIndex = nextReqIndex;
    nextReqIndex = (nextReqIndex + 1) % pipelineSize;

    pipeline[nextReqIndex].issuer = Component::NONE;
    pipeline[nextReqIndex].type = MemoryAccessType::NONE;
    pipeline[nextReqIndex].token = 0;
    pipeline[nextReqIndex].byteAddr = 0;

    DEBUG_CMD(DEBUG_MEMORY, printf("Memory: "));

    if (pipeline[nextRespIndex].issuer == Component::NONE)
    {
        /* There are no requests to serve */
        DEBUG_CMD(DEBUG_MEMORY, printf("No requests pending\n"));
        return 0;
    }
    else if (GET_WORD_INDEX(pipeline[nextRespIndex].byteAddr) >= memSizeWords)
    {
        fprintf(stderr,
                "%s:%s:%d: Out-of-bounds memory access to byteAddr "
                "0x%08" PRIX32 " (%" PRIu32 " words) of memSizeWords %" PRIu32
                "\n",
                __FILE__,
                __func__,
                __LINE__,
                pipeline[nextRespIndex].byteAddr,
                GET_WORD_INDEX(pipeline[nextRespIndex].byteAddr),
                memSizeWords);
        exit(1);
    }

    /* Serve the pending request */
    switch (pipeline[nextRespIndex].type)
    {
        case MemoryAccessType::LOAD:
            wordBaseAddr = getMemAccessWidthBaseByteAddr(
                pipeline[nextRespIndex].byteAddr);
            wordBaseAddr = GET_WORD_INDEX(wordBaseAddr);
            memcpy(pipeline[nextRespIndex].respData,
                   mem + wordBaseAddr,
                   memAccessWidthWords * sizeof(uint32_t));
            DEBUG_CMD(DEBUG_MEMORY, printf("Serving LOAD\n"));
            break;

        case MemoryAccessType::STORE:
            mem[GET_WORD_INDEX(pipeline[nextRespIndex].byteAddr)] =
                pipeline[nextRespIndex].reqData[0];
            DEBUG_CMD(DEBUG_MEMORY, printf("Serving STORE\n"));
            break;

        default:
            fprintf(stderr, "Invalid memory access request type\n");
            exit(1);
    }

    DEBUG_CMD(DEBUG_MEMORY, print());

    return 0;
}

void Memory::print()
{
    uint32_t i, j;
    uint32_t nextRespIndex =
        (nextReqIndex == 0) ? pipelineSize - 1 : nextReqIndex - 1;

    for (i = 0; i < pipelineSize; i++)
    {
        printf("    i: %" PRIu32 " token: %08" PRIX32 " type:%s "
               "byteAddr:%08" PRIX32 " issuer:%s",
               i,
               pipeline[i].token,
               memAccessTypeToStr(pipeline[i].type).c_str(),
               pipeline[i].byteAddr,
               componentToStr(pipeline[i].issuer).c_str());
        if (nextReqIndex == i)
        {
            printf(" <- nextReqIndex");
        }
        if (nextRespIndex == i)
        {
            printf(" <- nextRespIndex");
        }
        printf("\n");
        if (pipeline[i].issuer != Component::NONE &&
            pipeline[i].type == MemoryAccessType::LOAD)
        {
            for (j = 0; j < memAccessWidthWords; j++)
            {
                printf("        data:0x%08" PRIX32 "\n",
                       pipeline[i].respData[j]);
            }
        }
        else if (pipeline[i].issuer != Component::NONE &&
                 pipeline[i].type == MemoryAccessType::STORE)
        {
            printf("        data:0x%08" PRIX32 "\n", pipeline[i].reqData[0]);
        }
    }
}

void Memory::dump()
{
    uint32_t i;
    uint32_t byteAddr;

    printf("Memory: size:%" PRIu32 " words\n", memSizeWords);
    for (i = 0; i < memSizeWords; i++)
    {
        byteAddr = i * 4;
        printf("addr:0x%08" PRIX32 " (0d%08" PRIu32 ", byte:0x%08" PRIX32
               ") data:0x%08" PRIX32 "\n",
               i,
               i,
               byteAddr,
               mem[i]);
    }
}

std::string Memory::componentToStr(Component component)
{
    switch (component)
    {
        case Component::FETCH:
            return "FETCH";

        case Component::DECODE:
            return "DECODE";

        case Component::EXECUTE:
            return "EXECUTE";

        case Component::RESET:
            return "RESET";

        case Component::NONE:
            return "NONE";

        default:
            return "UNKNOWN";
    }
}

std::string Memory::memAccessTypeToStr(MemoryAccessType type)
{
    switch (type)
    {
        case MemoryAccessType::LOAD:
            return "LOAD";

        case MemoryAccessType::STORE:
            return "STORE";

        case MemoryAccessType::NONE:
            return "NONE";

        default:
            return "UNKNOWN";
    }
}

void Memory::loadWord(uint32_t byteAddr, uint32_t &data)
{
    if (GET_WORD_INDEX(byteAddr) >= memSizeWords)
    {
        fprintf(stderr,
                "%s:%s:%d: Out-of-bounds memory access to byteAddr "
                "0x%08" PRIX32 " (%" PRIu32 " words) of memSizeWords %" PRIu32
                "\n",
                __FILE__,
                __func__,
                __LINE__,
                byteAddr,
                GET_WORD_INDEX(byteAddr),
                memSizeWords);
        exit(1);
    }

    data = mem[GET_WORD_INDEX(byteAddr)];
}
