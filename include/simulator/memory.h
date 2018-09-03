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
#ifndef _MEMORY_H_
#define _MEMORY_H_

#include "simulator/config.h"
#include "simulator/utils.h"

#include <string>

enum class Component
{
    FETCH,
    DECODE,
    EXECUTE,
    RESET,
    NONE,
};

enum class MemoryAccessType
{
    LOAD,
    STORE,
    NONE,
};

class MemoryRequest
{
public:
    Component issuer;
    MemoryAccessType type;
    uint32_t token;
    uint32_t byteAddr;
    uint32_t *reqData;
    bool *reqEnable;
    uint32_t *respData;
};

class Memory
{
public:
    Memory(uint32_t memSizeWordsIn = MEM_SIZE_WORDS,
           uint32_t memAccessWidthWordsIn = MEM_ACCESS_WIDTH_WORDS,
           uint32_t pipelineSizeIn = MEM_PIPELINE_SIZE);
    ~Memory();

    /* Functions for booting */
    int loadProgram(char *programFile,
                    uint32_t &pc,
                    uint32_t &programByteSize);

    /* Convenience function for loading a word without interface */
    void loadWord(uint32_t byteAddr, uint32_t &data);

    /* Create a request in the memory access pipeline */
    int requestLoad(Component issuer, uint32_t byteAddr, uint32_t &token);
    int requestStore(Component issuer,
                     uint32_t byteAddr,
                     uint32_t data,
                     uint32_t &token);

    int retrieveLoad(uint32_t token, uint32_t &data);
    int retrieveStore(uint32_t token);
    int retrieveWideLoad(uint32_t token, uint32_t *data);

    bool isAvailable();

    int run();

    uint32_t getMemAccessWidthWordIndex(uint32_t byteAddr)
    {
        return GET_WORD_INDEX(byteAddr &
                              (BYTES_PER_WORD * memAccessWidthWords - 1));
    }

    uint32_t getMemAccessWidthBaseByteAddr(uint32_t byteAddr)
    {
        return byteAddr & ~(BYTES_PER_WORD * memAccessWidthWords - 1);
    }

    uint32_t getMemAccessWidthWords()
    {
        return memAccessWidthWords;
    }

    uint32_t getMemAccessWidthInstOffset(uint32_t byteAddr)
    {
        return (byteAddr & (memAccessWidthWords * BYTES_PER_WORD - 1)) /
            THUMB_INST_BYTES;
    }

    uint32_t getMemSizeWords()
    {
        return memSizeWords;
    }

    void print();
    void dump();

    static std::string componentToStr(Component component);
    static std::string memAccessTypeToStr(MemoryAccessType type);

private:
    uint32_t *mem{ nullptr };
    uint32_t memSizeWords;
    uint32_t memAccessWidthWords;

    MemoryRequest *pipeline{ nullptr };
    uint32_t pipelineSize;
    uint32_t nextReqIndex;
    uint32_t nextToken;
};

#endif /* _MEMORY_H_ */
