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

#include <cinttypes>
#include <cstdint>
#include <cstdlib>
#include <list>

void Execute::formatDataForMemLoad(MemoryInstructionType type,
                                   uint32_t &data,
                                   uint32_t offset)
{
    uint32_t byteOffset = GET_BYTE_INDEX(offset);

    switch (type)
    {
        case MemoryInstructionType::SBYTE:
            byteOffset = byteOffset * BITS_PER_BYTE;
            data = (data >> byteOffset) & ((0x1 << BITS_PER_BYTE) - 1);
            data |= (GET_BIT_AT_POS(data, 7) == 0x1) ? 0xFFFFFF00 : 0;
            break;

        case MemoryInstructionType::UBYTE:
            byteOffset = byteOffset * BITS_PER_BYTE;
            data = (data >> byteOffset) & ((0x1 << BITS_PER_BYTE) - 1);
            break;

        case MemoryInstructionType::SHALFWORD:
            byteOffset = (byteOffset & ~0x1) * BITS_PER_BYTE;
            data = (data >> byteOffset) & ((0x1 << BITS_PER_HALFWORD) - 1);
            data |= (GET_BIT_AT_POS(data, 15) == 0x1) ? 0xFFFF0000 : 0;
            break;

        case MemoryInstructionType::UHALFWORD:
            byteOffset = (byteOffset & ~0x1) * BITS_PER_BYTE;
            data = (data >> byteOffset) & ((0x1 << BITS_PER_HALFWORD) - 1);
            break;

        case MemoryInstructionType::WORD:
            /* Data and pflag remain unchanged here */
            break;
    }
}

void Execute::formatDataForMemStore(MemoryInstructionType type,
                                    uint32_t data,
                                    uint32_t &drt,
                                    uint32_t offset)
{
    uint32_t byteOffset = GET_BYTE_INDEX(offset);
    uint32_t mask;

    switch (type)
    {
        case MemoryInstructionType::SBYTE:
            fprintf(stderr,
                    "Invalid state, signed byte stores not "
                    "supported\n");
            exit(1);

        case MemoryInstructionType::UBYTE:
            byteOffset = byteOffset * BITS_PER_BYTE;
            mask = (0x1 << BITS_PER_BYTE) - 1;
            data = data & ~(mask << byteOffset);
            drt = data | ((drt & mask) << byteOffset);
            break;

        case MemoryInstructionType::SHALFWORD:
            fprintf(stderr,
                    "Invalid state, signed halftword stores not "
                    "supported\n");
            exit(1);

        case MemoryInstructionType::UHALFWORD:
            byteOffset = (byteOffset & ~0x1) * BITS_PER_BYTE;
            mask = (0x1 << BITS_PER_HALFWORD) - 1;
            data = data & ~(mask << byteOffset);
            drt = data | ((drt & mask) << byteOffset);
            break;

        case MemoryInstructionType::WORD:
            /* Data and pflag remain unchanged here */
            // drt = drt;
            break;
    }
}

int Execute::ldr(Reg rt,
                 uint32_t drn,
                 uint32_t offset,
                 MemoryInstructionType type)
{
    loadTmps.ptr = GET_WORD_ADDRESS(drn);
    loadTmps.byteOffset = GET_BYTE_INDEX(drn) + offset;
    loadTmps.type = type;
    loadTmps.destReg = rt;

    return executeLoadMemReq();
}

int Execute::ldr1Ldr4(Reg rt, uint32_t drn, uint32_t im)
{
    ldr(rt, drn, im << 2, MemoryInstructionType::WORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDR1 | LDR4\n"));
    return 0;
}

int Execute::ldr2(Reg rt, uint32_t drn, uint32_t drm)
{
    ldr(rt, drn, drm, MemoryInstructionType::WORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDR2\n"));
    return 0;
}

int Execute::ldr3(Reg rt, uint32_t drn, uint32_t im)
{
    drn = ALIGN(drn, BYTES_PER_WORD);
    ldr(rt, drn, im << 2, MemoryInstructionType::WORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDR3\n"));
    return 0;
}

int Execute::ldrb1(Reg rt, uint32_t drn, uint32_t im)
{
    ldr(rt, drn, im, MemoryInstructionType::UBYTE);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDRB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDRB1\n"));
    return 0;
}

int Execute::ldrb2(Reg rt, uint32_t drn, uint32_t drm)
{
    ldr(rt, drn, drm, MemoryInstructionType::UBYTE);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDRB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDRB2\n"));
    return 0;
}

int Execute::ldrh1(Reg rt, uint32_t drn, uint32_t im)
{
    ldr(rt, drn, im << 1, MemoryInstructionType::UHALFWORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDRH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDRH1\n"));
    return 0;
}

int Execute::ldrh2(Reg rt, uint32_t drn, uint32_t drm)
{
    ldr(rt, drn, drm, MemoryInstructionType::UHALFWORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDRH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDRH2\n"));
    return 0;
}

int Execute::ldrsb(Reg rt, uint32_t drn, uint32_t drm)
{
    ldr(rt, drn, drm, MemoryInstructionType::SBYTE);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDRSB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDRSB\n"));
    return 0;
}

int Execute::ldrsh(Reg rt, uint32_t drn, uint32_t drm)
{
    ldr(rt, drn, drm, MemoryInstructionType::SHALFWORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDRSH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" LDRSH\n"));
    return 0;
}

int Execute::str(Reg rt,
                 uint32_t drt,
                 Reg rn,
                 uint32_t drn,
                 uint32_t offset,
                 MemoryInstructionType type)
{
    storeTmps.ptr = GET_WORD_ADDRESS(drn);
    storeTmps.byteOffset = GET_BYTE_INDEX(drn) + offset;
    storeTmps.type = type;
    storeTmps.data = drt;
    storeTmps.dataReg = rt;
    storeTmps.addrReg = rn;

    return executeStoreMemReq();
}

int Execute::str1Str3(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t im)
{
    str(rt, drt, rn, drn, im << 2, MemoryInstructionType::WORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::STR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" STR1 | STR3\n"));
    return 0;
}

int Execute::str2(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t drm)
{
    str(rt, drt, rn, drn, drm, MemoryInstructionType::WORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::STR);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" STR2\n"));
    return 0;
}

int Execute::strb1(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t im)
{
    str(rt, drt, rn, drn, im, MemoryInstructionType::UBYTE);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::STRB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" STRB1\n"));
    return 0;
}

int Execute::strb2(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t drm)
{
    str(rt, drt, rn, drn, drm, MemoryInstructionType::UBYTE);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::STRB);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" STRB2\n"));
    return 0;
}

int Execute::strh1(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t im)
{
    str(rt, drt, rn, drn, im << 1, MemoryInstructionType::UHALFWORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::STRH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" STRH1\n"));
    return 0;
}

int Execute::strh2(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t drm)
{
    str(rt, drt, rn, drn, drm, MemoryInstructionType::UHALFWORD);

    /* Record the instruction stats */
    stats->addInstruction(Instruction::STRH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" STRH2\n"));
    return 0;
}

int Execute::popLdmia(Reg rn, uint32_t drn, uint32_t rl)
{
    mloadTmps.baseReg = rn;
    mloadTmps.ptr = drn;
    mloadTmps.byteOffset = 0;
    populateRegisterList(mloadTmps.regList, rl);

    executeMultipleLoadFirstMemReq();

    /* Record the instruction stats */
    stats->addInstruction(Instruction::LDMIA);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" POP | LDMIA\n"));
    return 0;
}

void Execute::populateRegisterList(std::list<Reg> &regList, uint32_t rl)
{
    uint32_t i;
    Reg reg;

    if (!regList.empty())
    {
        fprintf(stderr,
                "Starting multiple memory access without empty "
                "register list\n");
        exit(1);
    }

    for (i = 0; i < REGFILE_CORE_REGS_COUNT; i++)
    {
        if (GET_BIT_AT_POS(rl, i) == 0x0)
        {
            continue;
        }

        reg = RegFile::uint32ToReg(i);
        regList.push_back(reg);
    }

    if (regList.empty())
    {
        fprintf(stderr,
                "%s:%d:Multiple memory access instruction has empty "
                "register list\n",
                __func__,
                __LINE__);
        exit(1);
    }
}

int Execute::stmia(Reg rn, uint32_t drn, uint32_t rl)
{
    mstoreTmps.baseReg = rn;
    mstoreTmps.ptr = drn;
    mstoreTmps.byteOffset = 0;
    mstoreTmps.op = DecodedOperation::STMIA;
    populateRegisterList(mstoreTmps.regList, rl);

    executeMultipleStoreFirstMemReq();

    /* Record the instruction stats */
    stats->addInstruction(Instruction::STMIA);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" STMIA\n"));
    return 0;
}

int Execute::push(Reg rn, uint32_t drn, uint32_t rl)
{
    mstoreTmps.baseReg = rn;
    mstoreTmps.ptr = drn;
    mstoreTmps.byteOffset = 0;
    mstoreTmps.op = DecodedOperation::PUSH;
    populateRegisterList(mstoreTmps.regList, rl);

    executeMultipleStoreFirstMemReq();

    /* Record the instruction stats */
    stats->addInstruction(Instruction::PUSH);

    DEBUG_CMD(DEBUG_EXECUTE, printf(" PUSH\n"));
    return 0;
}
