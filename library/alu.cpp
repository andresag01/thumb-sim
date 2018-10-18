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
#include "simulator/fetch.h"
#include "simulator/regfile.h"

void Execute::calculateXpsrZ(uint32_t res)
{
    uint32_t xpsr = regFile->readData(Reg::XPSR);

    /* Calculate Z flag */
    xpsr = (res == 0) ? RegFile::setXpsrZ(xpsr, 0x1) :
                        RegFile::setXpsrZ(xpsr, 0x0);

    regFile->write(Reg::XPSR, xpsr);
}

void Execute::calculateXpsrN(uint32_t res, uint32_t bits)
{
    if (bits > 32 || bits < 1)
    {
        fprintf(stderr,
                "Cannot calculate XPSR flags for specified bit "
                "width\n");
        return;
    }

    uint32_t xpsr = regFile->readData(Reg::XPSR);

    /* Calculate N flag */
    xpsr = ((res >> (bits - 1)) == 0) ? RegFile::setXpsrN(xpsr, 0x0) :
                                        RegFile::setXpsrN(xpsr, 0x1);

    regFile->write(Reg::XPSR, xpsr);
}

void Execute::calculateXpsrQ()
{
    uint32_t xpsr = regFile->readData(Reg::XPSR);

    /* Calculate Q flag */
    xpsr = RegFile::setXpsrQ(xpsr, 0x0);

    regFile->write(Reg::XPSR, xpsr);
}

void Execute::calculateXpsrC(uint32_t res,
                             uint32_t op0,
                             uint32_t op1,
                             uint32_t cflag,
                             uint32_t bits)
{
    uint32_t msb = bits - 1;
    uint32_t mask = (bits < 32) ? (0x1 << bits) - 1 : ~0;
    uint32_t mask_msb = mask & ~(0x1 << msb);
    uint32_t tmp0;

    uint32_t xpsr = regFile->readData(Reg::XPSR);

    op0 &= mask;
    op1 &= mask;
    res &= mask;

    /* Calculate C flag */
    // Compute the carry out of the first (bits-1) bits
    tmp0 = (op0 & mask_msb) + (op1 & mask_msb) + cflag;
    // Compute the carry out of the full addition
    tmp0 = (op0 >> msb) + (op1 >> msb) + (tmp0 >> msb);
    xpsr = ((tmp0 & 0x2) != 0) ? RegFile::setXpsrC(xpsr, 0x1) :
                                 RegFile::setXpsrC(xpsr, 0x0);

    regFile->write(Reg::XPSR, xpsr);
}

void Execute::calculateXpsrFlags(uint32_t res,
                                 uint32_t op0,
                                 uint32_t op1,
                                 uint32_t cflag,
                                 uint32_t bits)
{
    if (bits > 32 || bits < 1)
    {
        fprintf(stderr,
                "Cannot calculate XPSR flags for specified bit "
                "width\n");
        return;
    }

    uint32_t msb = bits - 1;
    uint32_t mask = (bits < 32) ? (0x1 << bits) - 1 : ~0;
    uint32_t mask_msb = mask & ~(0x1 << msb);
    uint32_t tmp0, tmp1;
    uint32_t xpsr;

    calculateXpsrZ(res);
    calculateXpsrN(res, bits);
    calculateXpsrQ();

    xpsr = regFile->readData(Reg::XPSR);

    op0 &= mask;
    op1 &= mask;
    res &= mask;

    /* Calculate C flag */
    // Compute the carry out of the first (bits-1) bits
    tmp0 = (op0 & mask_msb) + (op1 & mask_msb) + cflag;
    // Compute the carry out of the full addition
    tmp0 = (op0 >> msb) + (op1 >> msb) + (tmp0 >> msb);
    xpsr = ((tmp0 & 0x2) != 0) ? RegFile::setXpsrC(xpsr, 0x1) :
                                 RegFile::setXpsrC(xpsr, 0x0);

    /* Calculate V flag */
    // Compute the carry out of the first (bits-1) bits
    tmp0 = ((op0 & mask_msb) + (op1 & mask_msb) + cflag) >> msb;
    // Compute the carry out of the full addition
    tmp1 = (tmp0 + (op0 >> msb) + (op1 >> msb)) >> 1;
    xpsr = (((tmp0 ^ tmp1) & 1) != 0) ? RegFile::setXpsrV(xpsr, 0x1) :
                                        RegFile::setXpsrV(xpsr, 0x0);

    regFile->write(Reg::XPSR, xpsr);
}

int Execute::adc(Reg rdn, uint32_t drdn, uint32_t drm, uint32_t cflag)
{
    uint32_t dres;

    dres = drdn + drm + cflag;
    calculateXpsrFlags(dres, drdn, drm, cflag, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ADC);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ADC\n"));
    return 0;
}

int Execute::add1(Reg rd, uint32_t drn, uint32_t im)
{
    uint32_t dres;

    dres = drn + im;
    calculateXpsrFlags(dres, drn, im, 0, BITS_PER_WORD);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ADD);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ADD1\n"));
    return 0;
}

int Execute::add2(Reg rdn, uint32_t drdn, uint32_t im)
{
    uint32_t dres;

    dres = drdn + im;
    calculateXpsrFlags(dres, drdn, im, 0, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ADD);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ADD2\n"));
    return 0;
}

int Execute::add3(Reg rd, uint32_t drn, uint32_t drm)
{
    uint32_t dres;

    dres = drn + drm;
    calculateXpsrFlags(dres, drn, drm, 0, BITS_PER_WORD);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ADD);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ADD3\n"));
    return 0;
}

int Execute::add4(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dres;

    dres = drdn + drm;

    regFile->write(rdn, dres);

    if (rdn == Reg::PC)
    {
        /*
         * I think that this instruction can jump to an unaligned address, so
         * just fail in that case
         */
        if (GET_BIT_AT_POS(dres, 0) != 0x0)
        {
            fprintf(stderr, "ADD4 branching to unaligned address\n");
            exit(1);
        }

        /* Flush the pipeline */
        flushPipeline();

        /* This is the equivalent of an "always taken" branch */
        stats->addBranchTaken();
        stats->addInstruction(Instruction::B);
    }
    else
    {
        /* Record the instruction stats */
        stats->addInstruction(Instruction::ADD);
    }

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ADD4\n"));
    return 0;
}

int Execute::add6Add7(Reg rd, uint32_t drm, uint32_t im)
{
    uint32_t dres;

    im = im << 2;
    dres = drm + im;

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ADD);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ADD5 | ADD6 | ADD7\n"));
    return 0;
}

int Execute::add5(Reg rd, uint32_t drm, uint32_t im)
{
    return add6Add7(rd, ALIGN(drm, BYTES_PER_WORD), im);
}

int Execute::and0(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dres;

    dres = drdn & drm;
    calculateXpsrZ(dres);
    calculateXpsrN(dres, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::AND);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" AND\n"));
    return 0;
}

int Execute::asr1(Reg rd, uint32_t drm, uint32_t im)
{
    uint32_t dxpsr;
    uint32_t dres;

    if (im >= BITS_PER_WORD)
    {
        /* ASR1 can only encode 5 bit immediates, so this should not happen */
        fprintf(stderr,
                "ASR1 received more than %u shift immediate\n",
                BITS_PER_WORD - 1);
        exit(1);
    }

    if (im == 0)
    {
        /* Shifting by 0, do nothing... */
        dres = drm;
    }
    else
    {
        /* Shift by the specified immediate */
        dres = drm >> im;
        if (GET_BIT_AT_POS(drm, BITS_PER_WORD - 1) == 0x1)
        {
            dres = dres | (~0U << (BITS_PER_WORD - im));
        }

        /* Set the carry bit to the value of the last bit shifted out */
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr = RegFile::setXpsrC(dxpsr, (drm >> (im - 1)) & 0x1);
        regFile->write(Reg::XPSR, dxpsr);
    }

    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrZ(dres);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ASR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ASR1\n"));
    return 0;
}

int Execute::asr2(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dxpsr;
    uint32_t dres;

    dxpsr = regFile->readData(Reg::XPSR);

    if (drm == 0)
    {
        /* Do nothing... */
        dres = drdn;
    }
    else if (drm < BITS_PER_WORD)
    {
        /*
         * Set the carry bit to the value of the last bit shifted
         * out
         */
        dxpsr = RegFile::setXpsrC(dxpsr, (drdn >> (drm - 1)) & 0x1);
        dres = drdn >> drm;
        if (GET_BIT_AT_POS(drdn, BITS_PER_WORD - 1) == 0x1)
        {
            dres = dres | (~0U << (BITS_PER_WORD - drm));
        }
    }
    else
    {
        /* Shift by 32 */
        if (GET_BIT_AT_POS(drdn, BITS_PER_WORD - 1) == 0x1)
        {
            dres = ~0;
            dxpsr = RegFile::setXpsrC(dxpsr, 0x1);
        }
        else
        {
            dres = 0;
            dxpsr = RegFile::setXpsrC(dxpsr, 0x0);
        }
    }

    regFile->write(Reg::XPSR, dxpsr);

    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrZ(dres);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ASR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ASR2\n"));
    return 0;
}

int Execute::bic(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dres;

    dres = drdn & ~drm;
    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrZ(dres);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::BIC);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" BIC\n"));
    return 0;
}

int Execute::cmn(uint32_t drn, uint32_t drm)
{
    uint32_t dres;

    dres = drn + drm;
    calculateXpsrFlags(dres, drn, drm, 0, BITS_PER_WORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::CMN);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" CMN\n"));
    return 0;
}

int Execute::cmp1(uint32_t drn, uint32_t im)
{
    uint32_t dres;

    dres = drn - im;

    calculateXpsrFlags(dres, drn, ~im, 1, BITS_PER_WORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::CMP);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" CMP1\n"));
    return 0;
}

int Execute::cmp2Cmp3(uint32_t drn, uint32_t drm)
{
    uint32_t dres;

    dres = drn - drm;

    calculateXpsrFlags(dres, drn, ~drm, 1, BITS_PER_WORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::CMP);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" CMP2 | CMP3\n"));
    return 0;
}

int Execute::cpy(Reg rd, uint32_t drm)
{
    if (rd == Reg::PC)
    {
        /*
         * When the destination is the pc this is a branch
         *
         * I think the reference manual is slightly unclear regarding whether
         * this instruction "exchanges" (as in the case of bx), but the
         * compiler is emiting code that does have the LSB set to 1 in the
         * address and then uses 'mov pc, rx' to jump there so I do not see a
         * difference between this an bx
         */
        return bx(rd, drm);
    }
    else
    {
        /* This is a regular reg-reg move */
        regFile->write(rd, drm);

        /* Record the instruction stats */
        stats->addInstruction(Instruction::MOV);

        DEBUG_CMD(DEBUG_EXECUTE, printf(" CPY\n"));
    }

    return 0;
}

int Execute::eor(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dres;

    dres = drdn ^ drm;
    calculateXpsrZ(dres);
    calculateXpsrN(dres, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::EOR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" EOR\n"));
    return 0;
}

int Execute::lsl1(Reg rd, uint32_t drm, uint32_t im)
{
    uint32_t dxpsr;
    uint32_t dres;

    if (im >= BITS_PER_WORD)
    {
        /* LSL1 can only encode 5 bit immediates, so this should not happen */
        fprintf(stderr,
                "LSL1 received more than %u shift immediate\n",
                BITS_PER_WORD - 1);
        exit(1);
    }

    if (im == 0)
    {
        /* Shifting by 0, do nothing... */
        dres = drm;
    }
    else
    {
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr = RegFile::setXpsrC(dxpsr, (drm >> (BITS_PER_WORD - im)) & 0x1);
        regFile->write(Reg::XPSR, dxpsr);
        dres = (im < BITS_PER_WORD) ? drm << im : drm;
    }

    calculateXpsrZ(dres);
    calculateXpsrN(dres, BITS_PER_WORD);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LSL);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LSL1\n"));
    return 0;
}

int Execute::lsl2(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dxpsr;
    uint32_t dres;

    if (drm == 0)
    {
        /* Shifting by 0, do nothing... */
        dres = drdn;
    }
    else if (drm == BITS_PER_WORD)
    {
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr = RegFile::setXpsrC(dxpsr, drdn & 0x1);
        regFile->write(Reg::XPSR, dxpsr);

        dres = 0;
    }
    else if (drm > BITS_PER_WORD)
    {
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr = RegFile::setXpsrC(dxpsr, 0);
        regFile->write(Reg::XPSR, dxpsr);

        dres = 0;
    }
    else
    {
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr =
            RegFile::setXpsrC(dxpsr, (drdn >> (BITS_PER_WORD - drm)) & 0x1);
        regFile->write(Reg::XPSR, dxpsr);
        dres = drdn << drm;
    }

    calculateXpsrZ(dres);
    calculateXpsrN(dres, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LSL);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LSL2\n"));
    return 0;
}

int Execute::lsr1(Reg rd, uint32_t drm, uint32_t im)
{
    uint32_t dxpsr;
    uint32_t dres;

    if (im >= BITS_PER_WORD)
    {
        /* LSR1 can only encode 5 bit immediates, so this should not happen */
        fprintf(stderr,
                "LSR1 received more than %u shift immediate\n",
                BITS_PER_WORD - 1);
        exit(1);
    }

    if (im == 0)
    {
        /* Shifting by 0, do nothing... */
        dres = drm;
    }
    else
    {
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr = RegFile::setXpsrC(dxpsr, GET_BIT_AT_POS(drm, im - 1));
        regFile->write(Reg::XPSR, dxpsr);
        dres = drm >> im;
    }

    calculateXpsrZ(dres);
    calculateXpsrN(dres, BITS_PER_WORD);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LSR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LSR1\n"));
    return 0;
}

int Execute::lsr2(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dxpsr;
    uint32_t dres;

    if (drm == 0)
    {
        /* Shifting by 0, do nothing... */
        dres = drdn;
    }
    else if (drm == BITS_PER_WORD)
    {
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr =
            RegFile::setXpsrC(dxpsr, GET_BIT_AT_POS(drdn, BITS_PER_WORD - 1));
        regFile->write(Reg::XPSR, dxpsr);

        dres = 0;
    }
    else if (drm > BITS_PER_WORD)
    {
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr = RegFile::setXpsrC(dxpsr, 0);
        regFile->write(Reg::XPSR, dxpsr);

        dres = 0;
    }
    else
    {
        dxpsr = regFile->readData(Reg::XPSR);
        dxpsr = RegFile::setXpsrC(dxpsr, GET_BIT_AT_POS(drdn, drm - 1));
        regFile->write(Reg::XPSR, dxpsr);
        dres = drdn >> drm;
    }

    calculateXpsrZ(dres);
    calculateXpsrN(dres, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LSR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LSR2\n"));
    return 0;
}

int Execute::mov1(Reg rd, uint32_t im)
{
    regFile->write(rd, im);
    calculateXpsrN(im, BITS_PER_WORD);
    calculateXpsrZ(im);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::MOV);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" MOV1\n"));
    return 0;
}

int Execute::mov2(Reg rd, uint32_t drm)
{
    calculateXpsrN(drm, BITS_PER_WORD);
    calculateXpsrZ(drm);

    regFile->write(rd, drm);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::MOV);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" MOV2\n"));
    return 0;
}

int Execute::mul(Reg rdn, uint32_t drdn, uint32_t drn)
{
    uint32_t dres;

    dres = drdn * drn;

    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrZ(dres);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::MUL);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" MUL\n"));
    return 0;
}

int Execute::mvn(Reg rd, uint32_t drm)
{
    uint32_t dres;

    dres = ~drm;

    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrZ(dres);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::MVN);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" MVN\n"));
    return 0;
}

int Execute::neg(Reg rd, uint32_t drn, uint32_t im)
{
    uint32_t dres;

    dres = im - drn;
    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrZ(dres);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::NEG);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" NEG\n"));
    return 0;
}

int Execute::orr(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dres;

    dres = drm | drdn;
    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrZ(dres);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ORR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ORR\n"));
    return 0;
}

int Execute::rev(Reg rd, uint32_t drm)
{
    uint32_t dres;

    dres = ((drm >> 0) & 0xFF) << 24;
    dres |= ((drm >> 8) & 0xFF) << 16;
    dres |= ((drm >> 16) & 0xFF) << 8;
    dres |= ((drm >> 24) & 0xFF) << 0;

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::REV);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" REV\n"));
    return 0;
}

int Execute::rev16(Reg rd, uint32_t drm)
{
    uint32_t dres;

    dres = ((drm >> 0) & 0xFF) << 8;
    dres |= ((drm >> 8) & 0xFF) << 0;
    dres |= ((drm >> 16) & 0xFF) << 24;
    dres |= ((drm >> 24) & 0xFF) << 16;

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::REV16);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" REV16\n"));
    return 0;
}

int Execute::revsh(Reg rd, uint32_t drm)
{
    uint32_t dres;

    dres = ((drm >> 0) & 0xFF) << 8;
    dres |= ((drm >> 8) & 0xFF) << 0;
    dres = (GET_BIT_AT_POS(dres, 15) == 0x1) ? dres | 0xFFFF0000 :
                                               dres | 0x0000FFFF;

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::REVSH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" REVSH\n"));
    return 0;
}

int Execute::ror(Reg rdn, uint32_t drdn, uint32_t drm)
{
    uint32_t dres;
    uint32_t dxpsr;

    if (drm == 0)
    {
        dres = drdn;
    }
    else
    {
        drm = drm % BITS_PER_WORD;
        dxpsr = regFile->readData(Reg::XPSR);
        if (drm == 0)
        {
            dxpsr = RegFile::setXpsrC(dxpsr, 0x1);
            dres = drdn;
        }
        else
        {
            dxpsr = RegFile::setXpsrC(dxpsr, drdn & (1 << (drm - 1)));
            dres = drdn << (BITS_PER_WORD - drm);
            dres |= drdn >> drm;
        }
        regFile->write(Reg::XPSR, dxpsr);
    }
    calculateXpsrZ(dres);
    calculateXpsrN(dres, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::ROR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" ROR\n"));
    return 0;
}

int Execute::sbc(Reg rdn, uint32_t drdn, uint32_t drm, uint32_t cflag)
{
    uint32_t dres;

    cflag = (cflag == 0x0) ? 0x1 : 0x0;
    dres = drdn - drm - cflag;
    calculateXpsrZ(dres);
    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrC(dres, drdn, ~drm, cflag, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::SBC);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" SBC\n"));
    return 0;
}

int Execute::sub1(Reg rd, uint32_t drn, uint32_t im)
{
    uint32_t dres;

    dres = drn - im;
    calculateXpsrFlags(dres, drn, ~im, 1, BITS_PER_WORD);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::SUB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" SUB1\n"));
    return 0;
}

int Execute::sub2(Reg rdn, uint32_t drdn, uint32_t im)
{
    uint32_t dres;

    dres = drdn - im;
    calculateXpsrFlags(dres, drdn, ~im, 1, BITS_PER_WORD);

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::SUB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" SUB2\n"));
    return 0;
}

int Execute::sub3(Reg rd, uint32_t drm, uint32_t drn)
{
    uint32_t dres;

    dres = drn - drm;
    calculateXpsrFlags(dres, drn, ~drm, 1, BITS_PER_WORD);

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::SUB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" SUB3\n"));
    return 0;
}

int Execute::sub4(Reg rdn, uint32_t drdn, uint32_t im)
{
    uint32_t dres;

    im = im << 2;

    dres = drdn - im;

    regFile->write(rdn, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::SUB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" SUB4\n"));
    return 0;
}

int Execute::tst(uint32_t drm, uint32_t drn)
{
    uint32_t dres;

    dres = drm & drn;

    calculateXpsrN(dres, BITS_PER_WORD);
    calculateXpsrZ(dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::TST);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" TST\n"));
    return 0;
}

int Execute::uxtb(Reg rd, uint32_t drm)
{
    uint32_t dres;

    dres = drm & 0xFF;

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::UXTB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" UXTB\n"));
    return 0;
}

int Execute::uxth(Reg rd, uint32_t drm)
{
    uint32_t dres;

    dres = drm & 0xFFFF;

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::UXTH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" UXTH\n"));
    return 0;
}

int Execute::sxtb(Reg rd, uint32_t drm)
{
    uint32_t dres;

    dres = drm & 0xFF;
    if (GET_BIT_AT_POS(dres, BITS_PER_BYTE - 1) != 0)
    {
        dres = dres | 0xFFFFFF00;
    }

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::SXTB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" SXTB\n"));
    return 0;
}

int Execute::sxth(Reg rd, uint32_t drm)
{
    uint32_t dres;

    dres = drm & 0xFFFF;
    if (GET_BIT_AT_POS(dres, BITS_PER_HALFWORD - 1) != 0)
    {
        dres = dres | 0xFFFF0000;
    }

    regFile->write(rd, dres);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::SXTH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" SXTH\n"));
    return 0;
}
