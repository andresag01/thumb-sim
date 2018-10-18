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
#include "simulator/decode.h"

#include "simulator/debug.h"
#include "simulator/utils.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>

Decode::Decode(Fetch *fetchInit, RegFile *regFileInit) :
    fetch(fetchInit),
    regFile(regFileInit)
{
}

uint32_t Decode::getCorrectedFetchAddress()
{
    uint32_t pc;
    regFile->read(Reg::PC, pc);
    return NEXT_THUMB_INST(pc);
}

void Decode::flush()
{
    flushPending = true;
}

DecodedInst *Decode::getNextInst()
{
    DecodedInst *inst = decodedInst;

    if (decodedHalfInst || decodedInst == nullptr)
    {
        return nullptr;
    }
    else
    {
        decodedInst = nullptr;
        return inst;
    }
}

void Decode::updateDecodedInstReg(DecodedInstRegIndex regIndex)
{
    Reg reg;
    uint32_t data;

    reg = decodedInst->getRegisterNumber(regIndex);
    if (reg == Reg::MSP || reg == Reg::PSP)
    {
        reg = regFile->getActiveSp();
    }

    if (reg != Reg::RNONE && reg != Reg::PC)
    {
        regFile->read(reg, data);
        decodedInst->setRegister(regIndex, reg, data);
    }
}

void Decode::updateDecodedInstRegs()
{
    updateDecodedInstReg(DecodedInstRegIndex::RD);
    updateDecodedInstReg(DecodedInstRegIndex::RT);
    updateDecodedInstReg(DecodedInstRegIndex::RDN);
    updateDecodedInstReg(DecodedInstRegIndex::RM);
    updateDecodedInstReg(DecodedInstRegIndex::RN);
    updateDecodedInstReg(DecodedInstRegIndex::XPSR);
}

/*
 * When the decode stage runs ahead of the execution, it is possible that a
 * value that does not correspond to an instruction is fetched. The decode
 * stage could try to interpret it as an instruction, which could cause an
 * error. To easily catch these problems, the decode stage puts in the pipeline
 * an SVC instruction that will stop the simulation unless there is a flush
 * command
 */
void Decode::issuePlaceholderInst()
{
    decodedInst->setOperation(DecodedOperation::SVC);
    decodedInst->setImmediate(66);
}

int Decode::run()
{
    uint16_t inst;
    uint32_t rd, rdn, rm, rn, rl, rt;
    uint32_t im32, im11, im10, im8, im7, im5, im3;
    uint32_t s, j1, j2, i1, i2;
    uint32_t cond;
    uint32_t ra, rb, rc, xpsr, pc;
    Reg activeSp;

    /*
     * This is the main decoder function that receives an integer value from
     * the fetch stage and performs some bitwise operations to:
     *      - Work out the instruction that needs to be executed
     *      - Extract register values (if any)
     *      - Extract immediate values (if any)
     * The code for this decoder reuses some code from David Welch's
     * thumbulator available at https://github.com/dwelch67/thumbulator
     */
    /* Copyright (c) 2010 David Welch dwelch@dwelch.com
     *
     * Permission is hereby granted, free of charge, to any person obtaining a
     * copy of this software and associated documentation files (the
     * "Software"), to deal in the Software without restriction, including
     * without limitation the rights to use, copy, modify, merge, publish,
     * distribute, sublicense, and/or sell copies of the Software, and to
     * permit persons to whom the Software is furnished to do so, subject to
     * the following conditions:
     *
     * The above copyright notice and this permission notice shall be included
     * in all copies or substantial portions of the Software.
     *
     * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
     * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
     * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
     * IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY
     * CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
     * TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
     * SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
     */

    if (flushPending)
    {
        /* Flushing the pipeline */

        decodedHalfInst = false;
        if (decodedInst != nullptr)
        {
            delete decodedInst;
            decodedInst = nullptr;
        }

        flushPending = false;

        DEBUG_CMD(DEBUG_DECODE, printf("Decode: flushing\n"));

        return 0;
    }
    else if (decodedInst != nullptr && !decodedHalfInst)
    {
        /* Update the registers as we might have loaded them a while ago */
        updateDecodedInstRegs();

        /* The previously decoded instruction has not been processed yet */
        DEBUG_CMD(DEBUG_DECODE,
                  printf("Decode: stalled, pending execution\n"));

        return 0;
    }

    /*
     * Try to get the next instruction, if there is none, then stall, if there
     * is it also lets the fetch stage know that we can progress
     */
    if (fetch->getNextInst(inst) != 0)
    {
        /* Stall as fetch could not provide the following instruction */
        DEBUG_CMD(DEBUG_DECODE, printf("Decode: stalled, pending fetch\n"));

        return 0;
    }
    else if (decodedInst == nullptr)
    {
        /* Allocate a new instruction if we are not in the middle of one */
        decodedInst = new DecodedInst();
    }

    pc = getCorrectedFetchAddress();
    activeSp = regFile->getActiveSp();

    DEBUG_CMD(DEBUG_DECODE, printf("Decode: "));

    /* In some cases the instruction are 32-bit, so process the second half */
    if (decodedHalfInst)
    {
        decodedHalfInst = false;

        /* A6.7.18 BL Encoding T1 */
        if ((inst & 0xD000) == 0xD000)
        {
            im11 = (inst >> 0) & 0x7FF;
            j1 = (inst >> 13) & 0x1;
            j2 = (inst >> 11) & 0x1;

            im32 = decodedInst->getImmediate();

            s = (im32 >> 24) & 0x1;
            i1 = ~(j1 ^ s) & 0x1;
            i2 = ~(j2 ^ s) & 0x1;

            im32 |= (i1 << 23) | (i2 << 22) | (im11 << 1);

            decodedInst->setImmediate(im32);

            DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
            return 0;
        }

        issuePlaceholderInst();

        DEBUG_CMD(DEBUG_DECODE,
                  printf("Unable to decode second half %04" PRIX16
                         ", issuing: ",
                         inst);
                  decodedInst->printDisassembly());

        return 0;
    }

    /* Main instruction decoder */

    /* A6.7.2 ADC (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4140)
    {
        rdn = (inst >> 0) & 0x07;
        rm = (inst >> 3) & 0x07;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);
        xpsr = regFile->readData(Reg::XPSR);

        decodedInst->setOperation(DecodedOperation::ADC);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);
        decodedInst->setRegister(DecodedInstRegIndex::XPSR, Reg::XPSR, xpsr);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.3 ADD(1) (immediate) Encoding T1 */
    if ((inst & 0xFE00) == 0x1C00)
    {
        rd = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        im3 = (inst >> 6) & 0x7;

        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::ADD1);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(im3);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.3 ADD(2) (immediate) Encoding T2 */
    if ((inst & 0xF800) == 0x3000)
    {
        rdn = (inst >> 8) & 0x7;
        im8 = (inst >> 0) & 0xFF;

        ra = regFile->readData(rdn);

        decodedInst->setOperation(DecodedOperation::ADD2);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.4 ADD(3) (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x1800)
    {
        rd = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::ADD3);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /*
     * A6.7.4 ADD(4) (register) Encoding T2
     * Also, implements A6.7.6 ADD (SP plus register Encodings T1 and T2)
     */
    if ((inst & 0xFF00) == 0x4400)
    {
        rdn = (inst >> 0) & 0x7;
        rdn |= (inst >> 4) & 0x8;
        rm = (inst >> 3) & 0xF;

        if (rdn == static_cast<uint32_t>(Reg::PC) && rdn == rm)
        {
            /* Unpredictable ADD4 operation, rdn == rm == pc */
            issuePlaceholderInst();

            DEBUG_CMD(DEBUG_DECODE,
                      printf("Unpredictable ADD4 operation rdn == rm == pc "
                             "(0x%04" PRIX16 "), issuing: ",
                             inst);
                      decodedInst->printDisassembly());

            return 0;
        }

        ra = (rdn == static_cast<uint32_t>(Reg::PC)) ? pc :
                                                       regFile->readData(rdn);
        rb = (rm == static_cast<uint32_t>(Reg::PC)) ? pc :
                                                      regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::ADD4);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.7 (ADR) ADD(5) (PC plus immediate) Encoding T1 */
    if ((inst & 0xF800) == 0xA000)
    {
        rd = (inst >> 8) & 0x7;
        im8 = (inst >> 0) & 0xFF;

        decodedInst->setOperation(DecodedOperation::ADD5);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, Reg::PC, pc);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.5 ADD(6) (SP plus immediate) Encoding T1 */
    if ((inst & 0xF800) == 0xA800)
    {
        rd = (inst >> 8) & 0x7;
        im8 = (inst >> 0) & 0xFF;

        rb = regFile->readData(activeSp);

        decodedInst->setOperation(DecodedOperation::ADD6);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, activeSp, rb);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.5 ADD(7) (SP plus immediate) Encoding T2 */
    if ((inst & 0xFF80) == 0xB000)
    {
        im7 = (inst >> 0) & 0x7F;

        rb = regFile->readData(activeSp);

        decodedInst->setOperation(DecodedOperation::ADD7);
        decodedInst->setRegister(DecodedInstRegIndex::RD, activeSp, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, activeSp, rb);
        decodedInst->setImmediate(im7);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.9 AND (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4000)
    {
        rdn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::AND);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.10 ASR(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x1000)
    {
        rd = (inst >> 0) & 0x07;
        rm = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::ASR1);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.11 ASR(2) (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4100)
    {
        rdn = (inst >> 0) & 0x07;
        rm = (inst >> 3) & 0x07;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::ASR2);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.12 B(1) (conditional branch) Encoding T1 */
    if ((inst & 0xF000) == 0xD000)
    {
        im8 = (inst >> 0) & 0xFF;
        cond = (inst >> 8) & 0xF;

        if (cond == static_cast<uint32_t>(DecodedCondition::U0))
        {
            /* Branch with undefined condition flags */
            issuePlaceholderInst();

            DEBUG_CMD(
                DEBUG_DECODE,
                printf("Branch with undefined condition flags (0x%04" PRIX16
                       "), issuing: ",
                       inst);
                decodedInst->printDisassembly());

            return 0;
        }
        else if (cond != static_cast<uint32_t>(DecodedCondition::U1))
        {
            xpsr = regFile->readData(Reg::XPSR);

            decodedInst->setOperation(DecodedOperation::B1);
            decodedInst->setRegister(DecodedInstRegIndex::RM, Reg::PC, pc);
            decodedInst->setRegister(
                DecodedInstRegIndex::XPSR, Reg::XPSR, xpsr);
            decodedInst->setImmediate(im8);
            decodedInst->setCondition(cond);

            DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
            return 0;
        }

        /* Go to SVC */
    }

    /* A6.7.12 B(2) (unconditional branch) Encoding T2 */
    if ((inst & 0xF800) == 0xE000)
    {
        im11 = (inst >> 0) & 0x7FF;

        decodedInst->setOperation(DecodedOperation::B2);
        decodedInst->setRegister(DecodedInstRegIndex::RM, Reg::PC, pc);
        decodedInst->setImmediate(im11);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.16 BIC (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4380)
    {
        rdn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::BIC);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.17 BKPT Encoding T1 */
    if ((inst & 0xFF00) == 0xBE00)
    {
        im8 = (inst >> 0) & 0xFF;

        decodedInst->setOperation(DecodedOperation::BKPT);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.18 BL (32-bit instruction) Encoding T1 */
    if ((inst & 0xF800) == 0xF000)
    {
        im10 = (inst >> 0) & 0x3FF;
        s = (inst >> 10) & 0x1;

        im32 = (im10 << 12) | (s << 24);

        decodedInst->setOperation(DecodedOperation::BL);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, Reg::PC, pc);
        decodedInst->setImmediate(im32);

        decodedHalfInst = true;

        DEBUG_CMD(DEBUG_DECODE, printf("bl first half\n"));
        return 0;
    }

    /* A6.7.19 BLX (register) Encoding T1 */
    if ((inst & 0xFF87) == 0x4780)
    {
        rm = (inst >> 3) & 0xF;

        if (rm == static_cast<uint32_t>(Reg::PC))
        {
            /* BLX cannot have PC as operand register */
            issuePlaceholderInst();

            DEBUG_CMD(
                DEBUG_DECODE,
                printf("BLX cannot have pc as operand register (0x%04" PRIX16
                       "), issuing: ",
                       inst);
                decodedInst->printDisassembly());

            return 0;
        }

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::BLX);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, Reg::PC, pc);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.20 BX Encoding T1 */
    if ((inst & 0xFF87) == 0x4700)
    {
        rm = (inst >> 3) & 0xF;

        rb = (rm == static_cast<uint32_t>(Reg::PC)) ? pc :
                                                      regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::BX);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, Reg::PC, pc);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.26 CMN (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x42C0)
    {
        rn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::CMN);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.27 CMP(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x2800)
    {
        rn = (inst >> 8) & 0x07;
        im8 = (inst >> 0) & 0xFF;

        ra = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::CMP1);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.28 CMP(2) (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4280)
    {
        rn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::CMP2);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.28 CMP(3) (register) Encoding T2 */
    if ((inst & 0xFF00) == 0x4500)
    {
        rn = (inst >> 0) & 0x7;
        rn |= (inst >> 4) & 0x8;
        rm = (inst >> 3) & 0xF;

        if (((inst >> 6) & 0x3) == 0x0)
        {
            /* Unpredictable CMP3 operation with two low registers */
            issuePlaceholderInst();

            DEBUG_CMD(DEBUG_DECODE,
                      printf("Unpredictable CMP3 operation with two low "
                             "registers (0x%04" PRIX16 "), issuing: ",
                             inst);
                      decodedInst->printDisassembly());

            return 0;
        }
        else if (rn == static_cast<uint32_t>(Reg::PC) ||
                 rm == static_cast<uint32_t>(Reg::PC))
        {
            /* Unpredictable CMP3 operation with pc operand */
            issuePlaceholderInst();

            DEBUG_CMD(DEBUG_DECODE,
                      printf("Unpredictable CMP3 operation with pc operand "
                             "(0x%04" PRIX16 "), issuing: ",
                             inst);
                      decodedInst->printDisassembly());

            return 0;
        }

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::CMP3);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.29 CPS Encoding T1 */
    if ((inst & 0xFFEC) == 0xB660)
    {
        /* Print the character in r0 */
        rm = 0;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::CPS);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.30 CPY (synonym of MOV), A6.7.76 MOV (register) Encoding T1 */
    if ((inst & 0xFF00) == 0x4600)
    {
        rd = (inst >> 0) & 0x7;
        rd |= (inst >> 4) & 0x8;
        rm = (inst >> 3) & 0xF;

        rb = (rm == static_cast<uint32_t>(Reg::PC)) ? pc :
                                                      regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::CPY);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.35 EOR (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4040)
    {
        rdn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::EOR);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.40 LDMIA Encoding T1 */
    if ((inst & 0xF800) == 0xC800)
    {
        rn = (inst >> 8) & 0x7;
        rl = (inst >> 0) & 0xFF;

        if (rl == 0)
        {
            /* Unpredictable LDMIA with 0 length register list */
            issuePlaceholderInst();

            DEBUG_CMD(DEBUG_DECODE,
                      printf("Unpredictable LDMIA with 0 length register list "
                             "(0x%04" PRIX16 "), issuing: ",
                             inst);
                      decodedInst->printDisassembly());

            return 0;
        }

        ra = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::LDMIA);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegisterList(rl);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.42 LDR(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x6800)
    {
        rt = (inst >> 0) & 0x07;
        rn = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::LDR1);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.44 LDR(2) (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x5800)
    {
        rt = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LDR2);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.43 LDR(3) (literal) Encoding T1 */
    if ((inst & 0xF800) == 0x4800)
    {
        rt = (inst >> 8) & 0x07;
        im8 = (inst >> 0) & 0xFF;

        decodedInst->setOperation(DecodedOperation::LDR3);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, Reg::PC, pc);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.42 LDR(4) (immediate) Encoding T2 */
    if ((inst & 0xF800) == 0x9800)
    {
        rt = (inst >> 8) & 0x07;
        im8 = (inst >> 0) & 0xFF;

        rb = regFile->readData(activeSp);

        decodedInst->setOperation(DecodedOperation::LDR4);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, activeSp, rb);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.45 LDRB(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x7800)
    {
        rt = (inst >> 0) & 0x07;
        rn = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::LDRB1);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.47 LDRB(2) (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x5C00)
    {
        rt = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LDRB2);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.54 LDRH(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x8800)
    {
        rt = (inst >> 0) & 0x07;
        rn = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::LDRH1);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.56 LDRH(2) (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x5A00)
    {
        rt = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LDRH2);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.60 LDRSB (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x5600)
    {
        rt = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LDRSB);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.64 LDRSH (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x5E00)
    {
        rt = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LDRSH);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.67 LSL(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x0000)
    {
        rd = (inst >> 0) & 0x07;
        rm = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LSL1);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.68 LSL(2) (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4080)
    {
        rdn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LSL2);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.69 LSR(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x0800)
    {
        rd = (inst >> 0) & 0x07;
        rm = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LSR1);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.70 LSR(2) (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x40C0)
    {
        rdn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::LSR2);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.75 MOV(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x2000)
    {
        rd = (inst >> 8) & 0x07;
        im8 = (inst >> 0) & 0xFF;

        decodedInst->setOperation(DecodedOperation::MOV1);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.76 MOV(2) (register) Encoding T2 */
    if ((inst & 0xFFC0) == 0x0000)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::MOV2);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.83 MUL Encoding T1 */
    if ((inst & 0xFFC0) == 0x4340)
    {
        rdn = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::MUL);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.85 MVN (register) */
    if ((inst & 0xFFC0) == 0x43C0)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::MVN);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.87 NEG (RSB immediate synonym), A6.7.106 RSB (immediate) Encoding
     * T1
     */
    if ((inst & 0xFFC0) == 0x4240)
    {
        rd = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;

        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::NEG);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(0);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.87 NOP Encoding T1 */
    if ((inst & 0xFFFF) == 0xBF00)
    {
        decodedInst->setOperation(DecodedOperation::NOP);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.91 ORR (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4300)
    {
        rdn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::ORR);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.97 POP Encoding T1 */
    if ((inst & 0xFE00) == 0xBC00)
    {
        rl = (inst >> 0) & 0xFF;
        rl |= ((inst >> 8) & 0x1) << static_cast<uint32_t>(Reg::PC);

        if (rl == 0)
        {
            /* Unpredictable POP with 0 length register list */
            issuePlaceholderInst();

            DEBUG_CMD(DEBUG_DECODE,
                      printf("Unpredictable POP with 0 length register list "
                             "(0x%04" PRIX16 "), issuing: ",
                             inst);
                      decodedInst->printDisassembly());

            return 0;
        }

        ra = regFile->readData(activeSp);

        decodedInst->setOperation(DecodedOperation::POP);
        decodedInst->setRegister(DecodedInstRegIndex::RN, activeSp, ra);
        decodedInst->setRegisterList(rl);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.98 PUSH Encoding T1 */
    if ((inst & 0xFE00) == 0xB400)
    {
        rl = (inst >> 0) & 0xFF;
        rl |= ((inst >> 8) & 0x1) << static_cast<uint32_t>(Reg::LR);

        if (rl == 0)
        {
            /* Unpredictable PUSH with 0 length register list */
            issuePlaceholderInst();

            DEBUG_CMD(DEBUG_DECODE,
                      printf("Unpredictable PUSH with 0 length register list "
                             "(0x%04" PRIX16 "), issuing: ",
                             inst);
                      decodedInst->printDisassembly());

            return 0;
        }

        ra = regFile->readData(activeSp);

        decodedInst->setOperation(DecodedOperation::PUSH);
        decodedInst->setRegister(DecodedInstRegIndex::RN, activeSp, ra);
        decodedInst->setRegisterList(rl);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.100 REV Encoding T1 */
    if ((inst & 0xFFC0) == 0xBA00)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::REV);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.101 REV16 Encoding T1 */
    if ((inst & 0xFFC0) == 0xBA40)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::REV16);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.102 REVSH Encoding T1 */
    if ((inst & 0xFFC0) == 0xBAC0)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::REV16);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.104 ROR (register) Encoding T1. Repurposed as GETM */
    if ((inst & 0xFFC0) == 0x41C0)
    {
        rdn = (inst >> 0) & 0x07;
        rm = (inst >> 3) & 0x07;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::ROR);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.109 SBC (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4180)
    {
        rdn = (inst >> 0) & 0x07;
        rm = (inst >> 3) & 0x07;

        ra = regFile->readData(rdn);
        rb = regFile->readData(rm);
        xpsr = regFile->readData(Reg::XPSR);

        decodedInst->setOperation(DecodedOperation::SBC);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);
        decodedInst->setRegister(DecodedInstRegIndex::XPSR, Reg::XPSR, xpsr);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.112 SEV Encoding T1 */
    if ((inst & 0xFFFF) == 0xBF40)
    {
        fprintf(stderr, "Unsupported instruction SEV\n");
        exit(1);
    }

    /* A6.7.117 STMIA Encoding T1 */
    if ((inst & 0xF800) == 0xC000)
    {
        rn = (inst >> 8) & 0x7;
        rl = (inst >> 0) & 0xFF;

        if (rl == 0)
        {
            /* Unpredictable STMIA with 0 length register list */
            issuePlaceholderInst();

            DEBUG_CMD(DEBUG_DECODE,
                      printf("Unpredictable STMIA with 0 length register list "
                             "(0x%04" PRIX16 "), issuing: ",
                             inst);
                      decodedInst->printDisassembly());

            return 0;
        }

        ra = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::STMIA);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegisterList(rl);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.119 STR(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x6000)
    {
        rt = (inst >> 0) & 0x07;
        rn = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        ra = regFile->readData(rt);
        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::STR1);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.120 STR(2) (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x5000)
    {
        rt = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);
        rc = regFile->readData(rt);

        decodedInst->setOperation(DecodedOperation::STR2);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, rc);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.119 STR(3) (immediate) Encoding T2 */
    if ((inst & 0xF800) == 0x9000)
    {
        rt = (inst >> 8) & 0x07;
        im8 = (inst >> 0) & 0xFF;

        ra = regFile->readData(rt);
        rb = regFile->readData(activeSp);

        decodedInst->setOperation(DecodedOperation::STR3);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RN, activeSp, rb);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.121 STRB(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x7000)
    {
        rt = (inst >> 0) & 0x07;
        rn = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        ra = regFile->readData(rt);
        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::STRB1);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.122 STRB(2) (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x5400)
    {
        rt = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);
        rc = regFile->readData(rt);

        decodedInst->setOperation(DecodedOperation::STRB2);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, rc);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.128 STRH(1) (immediate) Encoding T1 */
    if ((inst & 0xF800) == 0x8000)
    {
        rt = (inst >> 0) & 0x07;
        rn = (inst >> 3) & 0x07;
        im5 = (inst >> 6) & 0x1F;

        ra = regFile->readData(rt);
        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::STRH1);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(im5);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.129 STRH(2) (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x5200)
    {
        rt = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);
        rc = regFile->readData(rt);

        decodedInst->setOperation(DecodedOperation::STRH2);
        decodedInst->setRegister(DecodedInstRegIndex::RT, rt, rc);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.132 SUB(1) (immediate) Encoding T1 */
    if ((inst & 0xFE00) == 0x1E00)
    {
        rd = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        im3 = (inst >> 6) & 0x7;

        rb = regFile->readData(rn);

        decodedInst->setOperation(DecodedOperation::SUB1);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, rb);
        decodedInst->setImmediate(im3);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.132 SUB(2) (immediate) Encoding T2 */
    if ((inst & 0xF800) == 0x3800)
    {
        rdn = (inst >> 8) & 0x7;
        im8 = (inst >> 0) & 0xFF;

        ra = regFile->readData(rdn);

        decodedInst->setOperation(DecodedOperation::SUB2);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, rdn, ra);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.133 SUB(3) (register) Encoding T1 */
    if ((inst & 0xFE00) == 0x1A00)
    {
        rd = (inst >> 0) & 0x7;
        rn = (inst >> 3) & 0x7;
        rm = (inst >> 6) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::SUB3);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.134 SUB(4) (SP minus immediate) Encoding T1 */
    if ((inst & 0xFF80) == 0xB080)
    {
        im7 = (inst >> 0) & 0x7F;

        ra = regFile->readData(activeSp);

        decodedInst->setOperation(DecodedOperation::SUB4);
        decodedInst->setRegister(DecodedInstRegIndex::RDN, activeSp, ra);
        decodedInst->setImmediate(im7);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.136 SVC (formerly SWI) Encoding T1 */
    if ((inst & 0xFF00) == 0xDF00)
    {
        im8 = inst & 0xFF;

        decodedInst->setOperation(DecodedOperation::SVC);
        decodedInst->setImmediate(im8);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.137 SXTB Encoding T1 */
    if ((inst & 0xFFC0) == 0xB240)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::SXTB);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.138 SXTH Encoding T1 */
    if ((inst & 0xFFC0) == 0xB200)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::SXTH);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.143 TST (register) Encoding T1 */
    if ((inst & 0xFFC0) == 0x4200)
    {
        rn = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        ra = regFile->readData(rn);
        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::TST);
        decodedInst->setRegister(DecodedInstRegIndex::RN, rn, ra);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.149 UXTB Encoding T1 */
    if ((inst & 0xFFC0) == 0xB2C0)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::UXTB);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    /* A6.7.150 UXTH Encoding T1 */
    if ((inst & 0xFFC0) == 0xB280)
    {
        rd = (inst >> 0) & 0x7;
        rm = (inst >> 3) & 0x7;

        rb = regFile->readData(rm);

        decodedInst->setOperation(DecodedOperation::UXTH);
        decodedInst->setRegister(DecodedInstRegIndex::RD, rd, 0);
        decodedInst->setRegister(DecodedInstRegIndex::RM, rm, rb);

        DEBUG_CMD(DEBUG_DECODE, decodedInst->printDisassembly());
        return 0;
    }

    issuePlaceholderInst();

    DEBUG_CMD(
        DEBUG_DECODE,
        printf("Unable to decode instruction %04" PRIX16 ", issuing: ", inst);
        decodedInst->printDisassembly());

    return 0;
}

void DecodedInst::setOperation(DecodedOperation opIn)
{
    op = opIn;
}

void DecodedInst::setImmediate(uint32_t imIn)
{
    im = imIn;
}

void DecodedInst::setRegisterList(uint32_t regListIn)
{
    regList = regListIn;
}

DecodedOperation DecodedInst::getOperation()
{
    return op;
}

uint32_t DecodedInst::getImmediate()
{
    return im;
}

Reg DecodedInst::getRegisterNumber(DecodedInstRegIndex index)
{
    return regsNumber[static_cast<uint32_t>(index)];
}

uint32_t DecodedInst::getRegisterData(DecodedInstRegIndex index)
{
    return regsData[static_cast<uint32_t>(index)];
}

uint32_t DecodedInst::getRegisterList()
{
    return regList;
}

DecodedCondition DecodedInst::getCondition()
{
    return cond;
}

void DecodedInst::clear()
{
    size_t i;

    for (i = 0; i < REGFILE_LOW_REGS_COUNT; i++)
    {
        regsNumber[i] = Reg::RNONE;
    }
}

void DecodedInst::setRegister(DecodedInstRegIndex index,
                              uint32_t reg,
                              uint32_t data)
{
    regsNumber[static_cast<uint32_t>(index)] = RegFile::uint32ToReg(reg);
    regsData[static_cast<uint32_t>(index)] = data;
}

void DecodedInst::setRegister(DecodedInstRegIndex index,
                              Reg reg,
                              uint32_t data)
{
    regsNumber[static_cast<uint32_t>(index)] = reg;
    regsData[static_cast<uint32_t>(index)] = data;
}

void DecodedInst::setCondition(uint32_t condIn)
{
    if (condIn >= static_cast<uint32_t>(DecodedCondition::COUNT))
    {
        fprintf(stderr, "Invalid condition flag %" PRIu32 "\n", condIn);
        exit(1);
    }

    cond = static_cast<DecodedCondition>(condIn);
}

void DecodedInst::printDisassembly()
{
    uint32_t i;
    std::string rd = RegFile::regToStr(
        regsNumber[static_cast<uint32_t>(DecodedInstRegIndex::RD)]);
    std::string rt = RegFile::regToStr(
        regsNumber[static_cast<uint32_t>(DecodedInstRegIndex::RT)]);
    std::string rdn = RegFile::regToStr(
        regsNumber[static_cast<uint32_t>(DecodedInstRegIndex::RDN)]);
    std::string rm = RegFile::regToStr(
        regsNumber[static_cast<uint32_t>(DecodedInstRegIndex::RM)]);
    std::string rn = RegFile::regToStr(
        regsNumber[static_cast<uint32_t>(DecodedInstRegIndex::RN)]);
    std::string xpsr = RegFile::regToStr(
        regsNumber[static_cast<uint32_t>(DecodedInstRegIndex::XPSR)]);
    bool comma = false;

    switch (op)
    {
        case DecodedOperation::ADC:
            printf("adc %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::ADD1:
            printf("adds %s, %s, #%" PRIu32 "\n", rd.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::ADD2:
            printf("adds %s, #%" PRIu32 "\n", rdn.c_str(), im);
            break;

        case DecodedOperation::ADD3:
            printf("adds %s, %s, %s\n", rd.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::ADD4:
            printf("add %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::ADD5:
            printf("add %s, %s, #%" PRIu32 "\n", rd.c_str(), rm.c_str(), im);
            break;

        case DecodedOperation::ADD6:
            printf("add %s, %s, #%" PRIu32 "\n", rd.c_str(), rm.c_str(), im);
            break;

        case DecodedOperation::ADD7:
            printf("add %s, %s, #%" PRIu32 "\n", rd.c_str(), rm.c_str(), im);
            break;

        case DecodedOperation::AND:
            printf("ands %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::ASR1:
            printf("asrs %s, %s, #%" PRIu32 "\n", rd.c_str(), rm.c_str(), im);
            break;

        case DecodedOperation::ASR2:
            printf("asrs %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::B1:
            printf("b%s #%" PRIu32 "\n", getConditionString(cond).c_str(), im);
            break;

        case DecodedOperation::B2:
            printf("b #%" PRIu32 "\n", im);
            break;

        case DecodedOperation::BIC:
            printf("bics %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::BKPT:
            printf("bkpt %" PRIu32 "\n", im);
            break;

        case DecodedOperation::BL:
            printf("bl %" PRIu32 "\n", im);
            break;

        case DecodedOperation::BLX:
            printf("blx %s\n", rm.c_str());
            break;

        case DecodedOperation::BX:
            printf("bx %s\n", rm.c_str());
            break;

        case DecodedOperation::CMN:
            printf("cmns %s, %s\n", rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::CMP1:
            printf("cmp %s,#%" PRIu32 "\n", rn.c_str(), im);
            break;

        case DecodedOperation::CMP2:
            printf("cmps %s, %s\n", rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::CMP3:
            printf("cmps %s, %s\n", rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::CPS:
            printf("cps\n");
            break;

        case DecodedOperation::CPY:
            printf("cpy %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::EOR:
            printf("eors %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::LDMIA:
            printf("ldmia %s" PRIu32 " {", rn.c_str());
            for (i = 0; i < REGFILE_CORE_REGS_COUNT; i++)
            {
                if (((regList >> i) & 0x1) == 0)
                    continue;

                if (comma)
                {
                    printf(", ");
                }
                else
                {
                    comma = true;
                }
                printf("r%" PRIu32, i);
            }
            printf("}\n");
            break;

        case DecodedOperation::LDR1:
            printf("ldr %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::LDR2:
            printf("ldr %s, [%s, %s]\n", rt.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::LDR3:
            printf("ldr %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::LDR4:
            printf("ldr %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::LDRB1:
            printf(
                "ldrb %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::LDRB2:
            printf("ldrb %s, [%s, %s]\n", rt.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::LDRH1:
            printf(
                "ldrh %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::LDRH2:
            printf("ldrh %s, [%s, %s]\n", rt.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::LDRSB:
            printf("ldrsb %s, [%s, %s]\n", rt.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::LDRSH:
            printf("ldrsh %s, [%s, %s]\n", rt.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::LSL1:
            printf("lsls %s, %s, #%" PRIu32 "\n", rd.c_str(), rm.c_str(), im);
            break;

        case DecodedOperation::LSL2:
            printf("lsls %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::LSR1:
            printf("lsrs %s, %s, #%" PRIu32 "\n", rd.c_str(), rm.c_str(), im);
            break;

        case DecodedOperation::LSR2:
            printf("lsrs %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::MOV1:
            printf("movs %s, #%" PRIu32 "\n", rd.c_str(), im);
            break;

        case DecodedOperation::MOV2:
            printf("movs %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::MUL:
            printf("muls %s, %s\n", rdn.c_str(), rn.c_str());
            break;

        case DecodedOperation::MVN:
            printf("mvns %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::NEG:
            printf("negs %s, %s\n", rd.c_str(), rn.c_str());
            break;

        case DecodedOperation::NOP:
            printf("nop\n");
            break;

        case DecodedOperation::ORR:
            printf("orrs %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::POP:
            printf("pop {");
            for (i = 0; i < REGFILE_CORE_REGS_COUNT; i++)
            {
                if (((regList >> i) & 0x1) == 0)
                    continue;

                if (comma)
                {
                    printf(", ");
                }
                else
                {
                    comma = true;
                }
                printf("r%" PRIu32, i);
            }
            printf("}\n");
            break;

        case DecodedOperation::PUSH:
            printf("push {");
            for (i = 0; i < REGFILE_CORE_REGS_COUNT; i++)
            {
                if (((regList >> i) & 0x1) == 0)
                    continue;

                if (comma)
                {
                    printf(", ");
                }
                else
                {
                    comma = true;
                }
                printf("r%" PRIu32, i);
            }
            printf("}\n");
            break;

        case DecodedOperation::REV:
            printf("rev %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::REV16:
            printf("rev16 %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::REVSH:
            printf("revsh %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::ROR:
            printf("ror %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::SBC:
            printf("sbc %s, %s\n", rdn.c_str(), rm.c_str());
            break;

        case DecodedOperation::STMIA:
            printf("stmia %s {", rn.c_str());
            for (i = 0; i < REGFILE_CORE_REGS_COUNT; i++)
            {
                if (((regList >> i) & 0x1) == 0)
                    continue;

                if (comma)
                {
                    printf(", ");
                }
                else
                {
                    comma = true;
                }
                printf("r%" PRIu32, i);
            }
            printf("}\n");
            break;

        case DecodedOperation::STR1:
            printf("str %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::STR2:
            printf("str %s, [%s, %s]\n", rt.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::STR3:
            printf("str %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::STRB1:
            printf(
                "strb %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::STRB2:
            printf("strb %s, [%s, %s]\n", rt.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::STRH1:
            printf(
                "strh %s, [%s, #%" PRIu32 "]\n", rt.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::STRH2:
            printf("strh %s, [%s, %s]\n", rt.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::SUB1:
            printf("subs %s, %s, #%" PRIu32 "\n", rd.c_str(), rn.c_str(), im);
            break;

        case DecodedOperation::SUB2:
            printf("subs %s, #%" PRIu32 "\n", rdn.c_str(), im);
            break;

        case DecodedOperation::SUB3:
            printf("subs %s, %s, %s\n", rd.c_str(), rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::SUB4:
            printf("sub %s, #%" PRIu32 "\n", rdn.c_str(), im);
            break;

        case DecodedOperation::SVC:
            printf("svc %" PRIu32 "\n", im);
            break;

        case DecodedOperation::SXTB:
            printf("sxtb %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::SXTH:
            printf("sxth %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::TST:
            printf("tst %s, %s\n", rn.c_str(), rm.c_str());
            break;

        case DecodedOperation::UXTB:
            printf("uxtb %s, %s\n", rd.c_str(), rm.c_str());
            break;

        case DecodedOperation::UXTH:
            printf("uxth %s, %s\n", rd.c_str(), rm.c_str());
            break;

        default:
            fprintf(stderr, "Unrecognised instruction while disassembling\n");
            exit(1);
    }
}

std::string DecodedInst::getConditionString(DecodedCondition cond)
{
    switch (cond)
    {
        case DecodedCondition::EQ:
            return "eq";
        case DecodedCondition::NE:
            return "ne";
        case DecodedCondition::CS:
            return "cs";
        case DecodedCondition::CC:
            return "cc";
        case DecodedCondition::MI:
            return "mi";
        case DecodedCondition::PL:
            return "pl";
        case DecodedCondition::VS:
            return "vs";
        case DecodedCondition::VC:
            return "vc";
        case DecodedCondition::HI:
            return "hi";
        case DecodedCondition::LS:
            return "ls";
        case DecodedCondition::GE:
            return "ge";
        case DecodedCondition::LT:
            return "lt";
        case DecodedCondition::GT:
            return "gt";
        case DecodedCondition::LE:
            return "le";
        case DecodedCondition::U0:
            return "u0";
        case DecodedCondition::U1:
            return "u1";
        default:
            fprintf(stderr, "Invalid condition\n");
            exit(1);
    }
}
