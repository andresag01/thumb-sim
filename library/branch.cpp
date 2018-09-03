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
#include "simulator/memory.h"
#include "simulator/regfile.h"

bool Execute::checkCondition(DecodedCondition cond, uint32_t xpsr)
{
    uint32_t n = RegFile::getXpsrN(xpsr);
    uint32_t z = RegFile::getXpsrZ(xpsr);
    uint32_t c = RegFile::getXpsrC(xpsr);
    uint32_t v = RegFile::getXpsrV(xpsr);

    switch (cond)
    {
        case DecodedCondition::EQ:
            return (z == 0x1) ? true : false;

        case DecodedCondition::NE:
            return (z == 0x0) ? true : false;

        case DecodedCondition::CS:
            return (c == 0x1) ? true : false;

        case DecodedCondition::CC:
            return (c == 0x0) ? true : false;

        case DecodedCondition::MI:
            return (n == 0x1) ? true : false;

        case DecodedCondition::PL:
            return (n == 0x0) ? true : false;

        case DecodedCondition::VS:
            return (v == 0x1) ? true : false;

        case DecodedCondition::VC:
            return (v == 0x0) ? true : false;

        case DecodedCondition::HI:
            return ((c == 0x1) && (z == 0x0)) ? true : false;

        case DecodedCondition::LS:
            return ((c == 0x0) || (z == 0x1)) ? true : false;

        case DecodedCondition::GE:
            return (n == v) ? true : false;

        case DecodedCondition::LT:
            return (n != v) ? true : false;

        case DecodedCondition::GT:
            return ((z == 0x0) && (n == v)) ? true : false;

        case DecodedCondition::LE:
            return ((z == 0x1) || (n != v)) ? true : false;

        default:
            fprintf(stderr, "Invalid condition\n");
            exit(1);
    }
}

int Execute::b1(Reg rm,
                uint32_t drm,
                uint32_t im,
                uint32_t dxpsr,
                DecodedCondition cond)
{
    uint32_t dres;

    if (!checkCondition(cond, dxpsr))
    {
        /* Condition failed */
        stats->addBranchNotTaken();
    }
    else
    {
        /* Condition passed */
        stats->addBranchTaken();

        /* Sign extend the immediate */
        im = (GET_BIT_AT_POS(im, 7) == 0x0) ?
            im :
            im | (static_cast<uint32_t>(~0) << 7);
        dres = (im << 1) + drm;

        /* Write the pc */
        regFile->write(rm, dres);

        /* Flush the pipeline */
        flushPipeline();
    }

    /* Record the instruction stats */
    stats->addInstruction(Instruction::B);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" B1\n"));
    return 0;
}

int Execute::b2(Reg rm, uint32_t drm, uint32_t im)
{
    uint32_t dres;

    /* Sign extend the immediate */
    im = (GET_BIT_AT_POS(im, 10) == 0x0) ?
        im :
        im | (static_cast<uint32_t>(~0) << 10);
    dres = (im << 1) + drm;

    /* Write the pc */
    regFile->write(rm, dres);

    /* Flush the pipeline */
    flushPipeline();

    stats->addBranchTaken();

    /* Record the instruction stats */
    stats->addInstruction(Instruction::B);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" B2\n"));
    return 0;
}

/*
 * The manual for Cortex-M0 says that this instruction takes 4 cycles to
 * execute. I think this is because it is a 32-bit instruction and the first
 * cycle is to calculate the offset to jump to. However, in this simulator that
 * offset is calculated at the decode stage and forces the execute to stall for
 * 1 cycle while the actual execution of bl takes 3 cycles. Therefore, this
 * instruction respects the timings in the manual
 */
int Execute::bl(Reg rdn, uint32_t drdn, uint32_t im)
{
    uint32_t dres;

    /* Sign extend the immediate */
    im = (GET_BIT_AT_POS(im, 24) == 0x0) ?
        im :
        im | (static_cast<uint32_t>(~0) << 24);
    dres = im + drdn;

    regFile->write(Reg::LR, drdn | 0x1);
    regFile->write(rdn, dres);

    /* Flush the pipeline */
    flushPipeline();

    /* Record the instruction stats */
    stats->addInstruction(Instruction::BL);

    stats->addBranchTaken();

    DEBUG_CMD(DEBUG_EXECUTE, printf(" BL\n"));
    return 0;
}

int Execute::blx(Reg rdn, uint32_t drdn, uint32_t drm)
{
    if ((drm & 0x1) != 0x1)
    {
        fprintf(stderr, "BLX cannot branch to ARM mode\n");
        exit(1);
    }

    /* Write the target address into rdn/pc and store the previous pc in lr */
    regFile->write(rdn, drm & ~0x1);
    regFile->write(Reg::LR, PREV_THUMB_INST(drdn) | 0x1);

    flushPipeline();

    /* Record the instruction stats */
    stats->addInstruction(Instruction::BLX);
    stats->addBranchTaken();

    DEBUG_CMD(DEBUG_EXECUTE, printf(" BLX\n"));
    return 0;
}

int Execute::bx(Reg rdn, uint32_t drm)
{
    if ((drm & 0x1) != 0x1)
    {
        fprintf(stderr, "BX cannot branch to ARM mode\n");
        exit(1);
    }

    regFile->write(rdn, drm & ~0x1);

    flushPipeline();

    /* Record the instruction stats */
    stats->addInstruction(Instruction::BX);
    stats->addBranchTaken();

    DEBUG_CMD(DEBUG_EXECUTE, printf(" BX\n"));
    return 0;
}
