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
#ifndef _STATISTICS_H_
#define _STATISTICS_H_

#include <cstdint>
#include <string>
#include <unordered_map>

enum class Instruction
{
    ADC,
    ADD,
    AND,
    ASR,
    B,
    BIC,
    BL,
    BLX,
    BX,
    CMN,
    CMP,
    EOR,
    LDMIA,
    LDR,
    LDRB,
    LDRH,
    LDRSB,
    LDRSH,
    LSL,
    LSR,
    MVN,
    MOV,
    MUL,
    NEG,
    NOP,
    ORR,
    REV,
    REV16,
    REVSH,
    ROR,
    SBC,
    PUSH,
    STMIA,
    STR,
    STRB,
    STRH,
    SUB,
    SXTB,
    SXTH,
    TST,
    UXTB,
    UXTH,
};

struct EnumInstructionHash
{
    template<typename T>
    std::size_t operator()(T t) const
    {
        return static_cast<std::size_t>(t);
    }
};

class Statistics
{
public:
    void addCycle();
    void addFetchCycle();
    void addExecuteCycle();

    void addStallForDecodeCycle();

    void addBranchTaken();
    void addBranchNotTaken();

    void setProgramSizeBytes(uint32_t size);
    void setMemSizeWords(uint32_t size);
    void setMemAccessWidthWords(uint32_t size);

    void addInstruction(Instruction inst);

    static std::string getInstructionStr(Instruction inst);

    void print();

private:
    /* Total cycles */
    uint64_t cycles{ 0 };
    /* Cycles spent placing memory requests */
    uint64_t fetchMemCycles{ 0 };
    /* Cycles spent in memory instructions not including allocation */
    uint64_t executeMemCycles{ 0 };

    /* Cycles stalled due to unavailable decoded instruction */
    uint64_t stalledForDecodeCycles{ 0 };

    /* Program size not including header */
    uint32_t programSizeBytes{ 0 };

    /* Memory size */
    uint32_t memSizeWords{ 0 };
    /* Memory access width */
    uint32_t memAccessWidthWords{ 0 };

    /* Branches taken (including unconditional branches) */
    uint64_t branchTaken{ 0 };
    /* Branches not taken */
    uint64_t branchNotTaken{ 0 };

    /* Information about executed instructions */
    std::unordered_map<Instruction, uint64_t, EnumInstructionHash> instCount;
};

#endif /* _STATISTICS_H_ */
