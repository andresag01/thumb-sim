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
#ifndef _DECODE_H_
#define _DECODE_H_

#include "simulator/fetch.h"
#include "simulator/regfile.h"

#include <cstdio>
#include <cstdlib>
#include <string>

enum class DecodedOperation
{
    NOP,
    ADC,
    ADD1,
    ADD2,
    ADD3,
    ADD4,
    ADD5,
    ADD6,
    ADD7,
    AND,
    ASR1,
    ASR2,
    B1,
    B2,
    BIC,
    BKPT,
    BL,
    BLX,
    BX,
    CMN,
    CMP1,
    CMP2,
    CMP3,
    CPS,
    CPY,
    EOR,
    LDMIA,
    LDR1,
    LDR2,
    LDR3,
    LDR4,
    LDRB1,
    LDRB2,
    LDRH1,
    LDRH2,
    LDRSB,
    LDRSH,
    LSL1,
    LSL2,
    LSR1,
    LSR2,
    MOV1,
    MOV2,
    MUL,
    MVN,
    NEG,
    ORR,
    POP,
    PUSH,
    REV,
    REV16,
    REVSH,
    ROR,
    SBC,
    STMIA,
    STR1,
    STR2,
    STR3,
    STRB1,
    STRB2,
    STRH1,
    STRH2,
    SUB1,
    SUB2,
    SUB3,
    SUB4,
    SVC,
    SXTB,
    SXTH,
    TST,
    UXTB,
    UXTH,
};

enum class DecodedInstRegIndex
{
    RD = 0,
    RT = 1,
    RDN = 2,
    RM = 3,
    RN = 4,
    XPSR = 5,
    RCOUNT,
};

enum class DecodedCondition
{
    EQ = 0x0,
    NE = 0x1,
    CS = 0x2,
    CC = 0x3,
    MI = 0x4,
    PL = 0x5,
    VS = 0x6,
    VC = 0x7,
    HI = 0x8,
    LS = 0x9,
    GE = 0xA,
    LT = 0xB,
    GT = 0xC,
    LE = 0xD,
    U0 = 0xE,
    U1 = 0xF,
    COUNT = 0x10,
};

class DecodedInst
{
public:
    bool isDecoded(void);
    void setOperation(DecodedOperation opIn);
    DecodedOperation getOperation();
    void setImmediate(uint32_t imIn);
    uint32_t getImmediate();
    void clear();
    void setRegister(DecodedInstRegIndex index, uint32_t reg, uint32_t data);
    void setRegister(DecodedInstRegIndex index, Reg reg, uint32_t data);
    uint32_t getRegisterData(DecodedInstRegIndex index);
    Reg getRegisterNumber(DecodedInstRegIndex index);
    void setRegisterList(uint32_t regListIn);
    uint32_t getRegisterList();
    void setCondition(uint32_t cond);
    DecodedCondition getCondition();
    void printDisassembly();

    static std::string getConditionString(DecodedCondition cond);

private:
    bool pending{ false };
    DecodedOperation op{ DecodedOperation::NOP };
    Reg regsNumber[REGFILE_LOW_REGS_COUNT]{ Reg::RNONE };
    uint32_t regsData[REGFILE_LOW_REGS_COUNT];
    uint32_t im;
    uint32_t regList;
    DecodedCondition cond;
};

class Decode
{
public:
    Decode(Fetch *fetchInit, RegFile *regFileInit);
    DecodedInst *getNextInst();
    int run();
    void flush();

private:
    void issuePlaceholderInst();
    uint32_t getCorrectedFetchAddress();
    void updateDecodedInstReg(DecodedInstRegIndex regIndex);
    void updateDecodedInstRegs();

    bool decodedHalfInst{ false };
    bool flushPending{ false };

    DecodedInst *decodedInst{ nullptr };

    Fetch *fetch;
    RegFile *regFile;
};

#endif /* _DECODE_H_ */
