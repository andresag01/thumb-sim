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
#include "simulator/regfile.h"

#include "simulator/utils.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>

uint32_t RegFile::readData(Reg reg)
{
    return regs[static_cast<uint32_t>(reg)];
}

uint32_t RegFile::readData(uint32_t reg)
{
    return regs[reg];
}

void RegFile::write(Reg reg, uint32_t data)
{
    regs[static_cast<uint32_t>(reg)] = data;
}

void RegFile::read(Reg reg, uint32_t &data)
{
    data = regs[static_cast<uint32_t>(reg)];
}

Reg RegFile::uint32ToReg(uint32_t reg)
{
    if (reg < REGFILE_SIZE)
    {
        return static_cast<Reg>(reg);
    }

    fprintf(stderr, "Register number out-of-bounds\n");
    exit(1);
}

#define MAKE_GET_XPSR_BIT(bitName)                        \
    uint32_t RegFile::getXpsr##bitName(uint32_t xpsr)     \
    {                                                     \
        return (xpsr >> XPSR_##bitName##BIT_INDEX) & 0x1; \
    }

MAKE_GET_XPSR_BIT(N)
MAKE_GET_XPSR_BIT(Z)
MAKE_GET_XPSR_BIT(C)
MAKE_GET_XPSR_BIT(V)
MAKE_GET_XPSR_BIT(Q)
MAKE_GET_XPSR_BIT(T)

#define MAKE_SET_XPSR_BIT(bitName)                                   \
    uint32_t RegFile::setXpsr##bitName(uint32_t xpsr, uint32_t flag) \
    {                                                                \
        return (xpsr & ~(0x1 << XPSR_##bitName##BIT_INDEX)) |        \
            ((flag & 0x1) << XPSR_##bitName##BIT_INDEX);             \
    }

MAKE_SET_XPSR_BIT(N)
MAKE_SET_XPSR_BIT(Z)
MAKE_SET_XPSR_BIT(C)
MAKE_SET_XPSR_BIT(V)
MAKE_SET_XPSR_BIT(Q)
MAKE_SET_XPSR_BIT(T)

uint32_t RegFile::getXpsrException(uint32_t xpsr)
{
    uint32_t exceptionMask = (0x1 << XPSR_EXCEPTION_BIT_COUNT) - 1;
    return (xpsr >> XPSR_EXCEPTION_BIT_INDEX) & exceptionMask;
}

uint32_t RegFile::setXpsrException(uint32_t xpsr, uint32_t exceptionNum)
{
    uint32_t exceptionMask = ((0x1 << XPSR_EXCEPTION_BIT_COUNT) - 1)
        << XPSR_EXCEPTION_BIT_INDEX;
    exceptionNum = exceptionNum << XPSR_EXCEPTION_BIT_INDEX;
    return (xpsr & ~exceptionMask) | (exceptionNum & exceptionMask);
}

#define MAKE_GET_CONTROL_BIT(bitName)                        \
    uint32_t RegFile::getControl##bitName()                  \
    {                                                        \
        return (regs[static_cast<uint32_t>(Reg::CONTROL)] >> \
                CONTROL_SBIT_INDEX) &                        \
            0x1;                                             \
    }

MAKE_GET_CONTROL_BIT(P)
MAKE_GET_CONTROL_BIT(S)

#define MAKE_SET_CONTROL_BIT(bitName)                                  \
    void RegFile::setControl##bitName(uint32_t flag)                   \
    {                                                                  \
        uint32_t r = static_cast<uint32_t>(Reg::CONTROL);              \
        regs[r] = (regs[r] & ~(0x1 << CONTROL_##bitName##BIT_INDEX)) | \
            ((flag & 0x1) << CONTROL_##bitName##BIT_INDEX);            \
    }

MAKE_SET_CONTROL_BIT(P)
MAKE_SET_CONTROL_BIT(S)

Reg RegFile::getActiveSp()
{
    if (getControlS() == 0)
    {
        return Reg::MSP;
    }
    else
    {
        return Reg::PSP;
    }
}

void RegFile::print()
{
    printf("RegFile: Register file contents\n");
    print(Reg::R0);
    print(Reg::R1);
    print(Reg::R2);
    print(Reg::R3);
    print(Reg::R4);
    print(Reg::R5);
    print(Reg::R6);
    print(Reg::R7);
    print(Reg::R8);
    print(Reg::R9);
    print(Reg::R10);
    print(Reg::R11);
    print(Reg::R12);
    print(Reg::MSP);
    print(Reg::PSP);
    print(Reg::LR);
    print(Reg::PC);
    print(Reg::XPSR);
    print(Reg::CONTROL);
}

std::string RegFile::regToStr(Reg reg)
{
    switch (reg)
    {
        case Reg::R0:
            return "r0";

        case Reg::R1:
            return "r1";

        case Reg::R2:
            return "r2";

        case Reg::R3:
            return "r3";

        case Reg::R4:
            return "r4";

        case Reg::R5:
            return "r5";

        case Reg::R6:
            return "r6";

        case Reg::R7:
            return "r7";

        case Reg::R8:
            return "r8";

        case Reg::R9:
            return "r9";

        case Reg::R10:
            return "r10";

        case Reg::R11:
            return "r11";

        case Reg::R12:
            return "r12";

        case Reg::MSP:
            return "msp";

        case Reg::LR:
            return "lr";

        case Reg::PC:
            return "pc";

        case Reg::PSP:
            return "psp";

        case Reg::XPSR:
            return "xpsr";

        case Reg::CONTROL:
            return "control";

        case Reg::RNONE:
            return "rnone";

        default:
            return "unknown";
    }
}

void RegFile::print(Reg reg)
{
    std::string prefix = "    ";
    uint32_t r = static_cast<uint32_t>(reg);

    printf("%s%-7s:0x%08" PRIX32 "\n",
           prefix.c_str(),
           regToStr(reg).c_str(),
           regs[r]);
}
