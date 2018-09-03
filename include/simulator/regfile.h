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
#ifndef _REGFILE_H_
#define _REGFILE_H_

#include <cstdint>
#include <string>

#define REGFILE_SIZE 19
#define REGFILE_LOW_REGS_COUNT 8
#define REGFILE_HIGH_REGS_COUNT 8
#define REGFILE_CORE_REGS_COUNT \
    (REGFILE_LOW_REGS_COUNT + REGFILE_HIGH_REGS_COUNT)
#define REGFILE_ROOT_REGS_COUNT (REGFILE_CORE_REGS_COUNT + 1)

#define XPSR_NBIT_INDEX 31
#define XPSR_ZBIT_INDEX 30
#define XPSR_CBIT_INDEX 29
#define XPSR_VBIT_INDEX 28
#define XPSR_QBIT_INDEX 27
#define XPSR_TBIT_INDEX 24
#define XPSR_EXCEPTION_BIT_INDEX 0
#define XPSR_EXCEPTION_BIT_COUNT 9

#define CONTROL_PBIT_INDEX 0
#define CONTROL_SBIT_INDEX 1

enum class Reg
{
    R0 = 0,
    R1 = 1,
    R2 = 2,
    R3 = 3,
    R4 = 4,
    R5 = 5,
    R6 = 6,
    R7 = 7,
    R8 = 8,
    R9 = 9,
    R10 = 10,
    R11 = 11,
    R12 = 12,
    MSP = 13,
    LR = 14,
    PC = 15,
    PSP = 16,
    XPSR = 17,
    CONTROL = 18,
    RNONE,
};

class RegFile
{
public:
    uint32_t readData(uint32_t reg);
    uint32_t readData(Reg reg);
    void read(Reg reg, uint32_t &data);

    void write(Reg reg, uint32_t data);

    void setControlS(uint32_t flag);
    void setControlP(uint32_t flag);

    uint32_t getControlS();
    uint32_t getControlP();

    Reg getActiveSp();

    void print();
    void print(Reg reg);
    static std::string regToStr(Reg reg);

    static Reg uint32ToReg(uint32_t reg);

    static uint32_t getXpsrN(uint32_t xpsr);
    static uint32_t getXpsrZ(uint32_t xpsr);
    static uint32_t getXpsrC(uint32_t xpsr);
    static uint32_t getXpsrV(uint32_t xpsr);
    static uint32_t getXpsrQ(uint32_t xpsr);
    static uint32_t getXpsrT(uint32_t xpsr);
    static uint32_t getXpsrException(uint32_t xpsr);

    static uint32_t setXpsrN(uint32_t xpsr, uint32_t flag);
    static uint32_t setXpsrZ(uint32_t xpsr, uint32_t flag);
    static uint32_t setXpsrC(uint32_t xpsr, uint32_t flag);
    static uint32_t setXpsrV(uint32_t xpsr, uint32_t flag);
    static uint32_t setXpsrQ(uint32_t xpsr, uint32_t flag);
    static uint32_t setXpsrT(uint32_t xpsr, uint32_t flag);
    static uint32_t setXpsrException(uint32_t xpsr, uint32_t exceptionNum);

private:
    uint32_t regs[REGFILE_SIZE]{ 0 };
};

#endif /* _REGFILE_H_ */
