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
#include "simulator/decode.h"
#include "simulator/fetch.h"
#include "simulator/memory.h"
#include "simulator/regfile.h"
#include "simulator/utils.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <list>
#include <string>

Execute::Execute(Fetch *fetchIn,
                 Decode *decodeIn,
                 RegFile *regFileIn,
                 Memory *memIn,
                 Statistics *statsIn) :
    regFile(regFileIn),
    decode(decodeIn),
    fetch(fetchIn),
    mem(memIn),
    stats(statsIn)
{
}

void Execute::flushPipeline()
{
    decode->flush();
    fetch->flush();
}

bool Execute::isStalled()
{
    /*
     * This function is used by the fetch stage to tell whether the pipeline is
     * stalled because of the execute stage (e.g. there is a multi-cycle
     * instruction)
     */

    if (execState == ExecuteState::NEXT_INST && decodedInst == nullptr)
    {
        return false;
    }
    else
    {
        return true;
    }
}

int Execute::executeMultipleStoreFirstMemReq()
{
    uint32_t regListByteSize = WORD_TO_BYTE_SIZE(mstoreTmps.regList.size());
    uint32_t endByteOffset;

    if (!mem->isAvailable())
    {
        fprintf(stderr, "Unexpected unavailable memory and directory\n");
        exit(1);
        ///* Memory is busy, try again later */
        // execState = ExecuteState::MULTIPLE_STORE_FIRST_MEM_REQ;
        // return 0;
    }

    /*
     * We have to do this because the PUSH operation moves the base pointer
     * before actually storing anything
     */
    if (mstoreTmps.op == DecodedOperation::STMIA)
    {
        endByteOffset = mstoreTmps.byteOffset + regListByteSize;
    }
    else if (mstoreTmps.op == DecodedOperation::PUSH)
    {
        endByteOffset = mstoreTmps.byteOffset - regListByteSize;
        mstoreTmps.byteOffset = endByteOffset;
    }
    else
    {
        fprintf(stderr, "Inconsistent instruction in %s\n", __func__);
        exit(1);
    }

    /* Update the base pointer to 1 element after the data stored */
    regFile->write(mstoreTmps.baseReg, mstoreTmps.ptr + endByteOffset);

    requestNextStore();

    execState = ExecuteState::MULTIPLE_STORE_MEM_REQ;
    return 0;
}

int Execute::executeMultipleStoreMemReq()
{
    /* Wait for the store to complete */
    if (mem->retrieveStore(mstoreTmps.memToken) != 0)
    {
        fprintf(stderr, "Failed memory response when expected\n");
        exit(1);
    }

    /* Store the next element */
    if (mstoreTmps.regList.size() > 0)
    {
        if (!mem->isAvailable())
        {
            fprintf(stderr, "Unexpected unavailable memory\n");
            exit(1);
            ///* Memory is busy, try again later */
            // execState = ExecuteState::MULTIPLE_STORE_MEM_REQ;
            // return 0;
        }
        /*
         * requestNextStore does not always require the directory, so there is
         * an opportunity to optimise this in the case that we needed to
         * update the deep and mark flags, but a directory load is not needed
         */
        requestNextStore();
        execState = ExecuteState::MULTIPLE_STORE_MEM_REQ;
    }
    else
    {
        execState = ExecuteState::NEXT_INST;
    }

    return 0;
}

int Execute::requestNextStore()
{
    int ret;
    uint32_t byteAddr = mstoreTmps.ptr + mstoreTmps.byteOffset;

    mstoreTmps.srcReg = mstoreTmps.regList.front();
    mstoreTmps.regList.pop_front();
    regFile->read(mstoreTmps.srcReg, mstoreTmps.data);

    ret = mem->requestStore(
        Component::EXECUTE, byteAddr, mstoreTmps.data, mstoreTmps.memToken);
    if (ret != 0)
    {
        fprintf(stderr, "Memory request failed when available\n");
        exit(1);
    }
    mstoreTmps.byteOffset = mstoreTmps.byteOffset + BYTES_PER_WORD;

    return 0;
}

int Execute::executeMultipleLoadFirstMemReq()
{
    uint32_t regListByteSize = WORD_TO_BYTE_SIZE(mloadTmps.regList.size());
    uint32_t byteAddr = mloadTmps.ptr + mloadTmps.byteOffset;
    int ret;

    if (!mem->isAvailable())
    {
        fprintf(stderr, "Unexpected unavailable memory\n");
        exit(1);
        ///* Memory is busy, try again later */
        // execState = ExecuteState::MULTIPLE_LOAD_FIRST_MEM_REQ;
        // return 0;
    }

    /* Update the base pointer to 1 element after the data loaded */
    regFile->write(mloadTmps.baseReg,
                   mloadTmps.ptr + mloadTmps.byteOffset + regListByteSize);

    ret = mem->requestLoad(Component::EXECUTE, byteAddr, mloadTmps.memToken);
    if (ret != 0)
    {
        fprintf(stderr, "Multiple memory request failed when available\n");
        exit(1);
    }
    mloadTmps.byteOffset = mloadTmps.byteOffset + BYTES_PER_WORD;

    execState = ExecuteState::MULTIPLE_LOAD_MEM_REQ;
    return 0;
}

int Execute::executeMultipleLoadMemReq()
{
    uint32_t byteAddr = mloadTmps.ptr + mloadTmps.byteOffset;
    int ret;

    /* Get the previous data and update the target register */
    if (mem->retrieveLoad(mloadTmps.memToken, mloadTmps.data) != 0)
    {
        fprintf(stderr, "Memory response not available when expected\n");
        exit(1);
        // execState = ExecuteState::MULTIPLE_LOAD_MEM_REQ;
        // return 0;
    }
    mloadTmps.destReg = mloadTmps.regList.front();
    mloadTmps.regList.pop_front();
    if (mloadTmps.destReg == Reg::PC)
    {
        regFile->write(mloadTmps.destReg, mloadTmps.data & ~0x1);

        execState = ExecuteState::FLUSH_PIPELINE;

        stats->addBranchTaken();

        /* Sanity check */
        if (mloadTmps.regList.size() > 0)
        {
            fprintf(stderr,
                    "pc is not the last register in multiple memory "
                    "load\n");
            exit(1);
        }

        return 0;
    }
    else
    {
        regFile->write(mloadTmps.destReg, mloadTmps.data);
    }

    /* Load the next element */
    if (mloadTmps.regList.size() > 0)
    {
        if (!mem->isAvailable())
        {
            fprintf(stderr, "Unexpected memory unavailable\n");
            exit(1);
            ///* Memory is busy, try again later */
            // execState = ExecuteState::MULTIPLE_LOAD_MEM_REQ;
            // return 0;
        }

        ret =
            mem->requestLoad(Component::EXECUTE, byteAddr, mloadTmps.memToken);
        if (ret != 0)
        {
            fprintf(stderr, "Multiple memory request failed when available\n");
            exit(1);
        }
        mloadTmps.byteOffset = mloadTmps.byteOffset + BYTES_PER_WORD;

        execState = ExecuteState::MULTIPLE_LOAD_MEM_REQ;
    }
    else
    {
        /*
         * This looks like we are going to spend an extra cycle just deciding
         * when to go to the next instruction, but in fact, this is needed
         * because after the last load request we need to spend 1 cycle
         * retrieving the data and writing it back to the register
         */
        execState = ExecuteState::NEXT_INST;
    }

    return 0;
}

int Execute::executeFlushPipeline()
{
    flushPipeline();

    execState = ExecuteState::NEXT_INST;

    return 0;
}

int Execute::executeLoadMemReq()
{
    int ret;
    uint32_t byteAddr = loadTmps.ptr + loadTmps.byteOffset;

    if (!mem->isAvailable())
    {
        fprintf(stderr, "Unexpected memory unavailable\n");
        exit(1);
        /* Memory is busy, try again later */
        // execState = ExecuteState::LOAD_MEM_REQ;
        // return 0;
    }

    ret = mem->requestLoad(Component::EXECUTE, byteAddr, loadTmps.memToken);
    if (ret != 0)
    {
        fprintf(stderr, "Memory request failed when available\n");
        exit(1);
    }

    execState = ExecuteState::LOAD_MEM_RESP;
    return 0;
}

int Execute::executeLoadMemResp()
{
    int ret;

    ret = mem->retrieveLoad(loadTmps.memToken, loadTmps.data);
    if (ret != 0)
    {
        fprintf(stderr, "Failed memory response when expected\n");
        exit(1);
        ///* The loaded data is not yet ready */
        // return 0;
    }

    /* Format the data according to the instruction */
    Execute::formatDataForMemLoad(
        loadTmps.type, loadTmps.data, loadTmps.byteOffset);

    if (loadTmps.destReg == Reg::PC)
    {
        fprintf(stderr, "Cannot load into pc\n");
        exit(1);
    }

    /* Write back the register */
    regFile->write(loadTmps.destReg, loadTmps.data);

    execState = ExecuteState::NEXT_INST;

    return 0;
}

int Execute::executeStoreMemReq()
{
    int ret;
    uint32_t byteAddr = storeTmps.ptr + storeTmps.byteOffset;
    uint32_t prevData;

    if (!mem->isAvailable())
    {
        fprintf(stderr, "Unexpected directory or memory unavailable\n");
        exit(1);
        ///* Memory is busy, try again later */
        // execState = ExecuteState::STORE_MEM_REQ;
        // return 0;
    }

    mem->loadWord(byteAddr, prevData);
    formatDataForMemStore(
        storeTmps.type, prevData, storeTmps.data, storeTmps.byteOffset);

    ret = mem->requestStore(
        Component::EXECUTE, byteAddr, storeTmps.data, storeTmps.memToken);
    if (ret != 0)
    {
        fprintf(stderr, "Memory request failed when available\n");
        exit(1);
    }

    execState = ExecuteState::STORE_MEM_RESP;
    return 0;
}

int Execute::executeStoreMemResp()
{
    if (mem->retrieveStore(storeTmps.memToken) != 0)
    {
        fprintf(stderr, "Failed memory response when expected\n");
        exit(1);
    }

    execState = ExecuteState::NEXT_INST;
    return 0;
}

void Execute::calculateExecCycles()
{
    switch (curExecState)
    {
        case ExecuteState::NEXT_INST:
            switch (execState)
            {
                case ExecuteState::NEXT_INST:
                    break;

                case ExecuteState::LOAD_MEM_REQ:
                case ExecuteState::LOAD_MEM_RESP:
                case ExecuteState::STORE_MEM_REQ:
                case ExecuteState::STORE_MEM_RESP:
                case ExecuteState::MULTIPLE_LOAD_FIRST_MEM_REQ:
                case ExecuteState::MULTIPLE_LOAD_MEM_REQ:
                case ExecuteState::MULTIPLE_STORE_FIRST_MEM_REQ:
                case ExecuteState::MULTIPLE_STORE_MEM_REQ:
                    stats->addExecuteCycle();
                    break;

                case ExecuteState::FLUSH_PIPELINE:
                    fprintf(stderr,
                            "Invalid state transition NEXT_INST -> "
                            "FLUSH_PIPELINE\n");
                    exit(1);
            }
            break;

        case ExecuteState::LOAD_MEM_REQ:
        case ExecuteState::LOAD_MEM_RESP:
        case ExecuteState::STORE_MEM_REQ:
        case ExecuteState::STORE_MEM_RESP:
        case ExecuteState::MULTIPLE_LOAD_FIRST_MEM_REQ:
        case ExecuteState::MULTIPLE_LOAD_MEM_REQ:
        case ExecuteState::MULTIPLE_STORE_FIRST_MEM_REQ:
        case ExecuteState::MULTIPLE_STORE_MEM_REQ:
            stats->addExecuteCycle();
            break;

        case ExecuteState::FLUSH_PIPELINE:
            /* This does not involve memory or directory usage for execute */
            break;
    }
}

int Execute::run()
{
    curExecState = execState;

    switch (execState)
    {
        case ExecuteState::NEXT_INST:
            executeNextInst();
            break;

        /* States for load */
        case ExecuteState::LOAD_MEM_REQ:
            executeLoadMemReq();
            break;

        case ExecuteState::LOAD_MEM_RESP:
            executeLoadMemResp();
            break;

        /* States for store */
        case ExecuteState::STORE_MEM_REQ:
            executeStoreMemReq();
            break;

        case ExecuteState::STORE_MEM_RESP:
            executeStoreMemResp();
            break;

        /* States for multiple load */
        case ExecuteState::MULTIPLE_LOAD_FIRST_MEM_REQ:
            executeMultipleLoadFirstMemReq();
            break;

        case ExecuteState::MULTIPLE_LOAD_MEM_REQ:
            executeMultipleLoadMemReq();
            break;

        /* States for multiple store */
        case ExecuteState::MULTIPLE_STORE_FIRST_MEM_REQ:
            executeMultipleStoreFirstMemReq();
            break;

        case ExecuteState::MULTIPLE_STORE_MEM_REQ:
            executeMultipleStoreMemReq();
            break;

        case ExecuteState::FLUSH_PIPELINE:
            executeFlushPipeline();
            break;
    }

    calculateExecCycles();

    DEBUG_CMD(DEBUG_EXECUTE,
              printf("Execute: %s -> %s\n",
                     execStateToStr(curExecState).c_str(),
                     execStateToStr(execState).c_str()));

    return 0;
}

std::string Execute::execStateToStr(ExecuteState state)
{
    switch (state)
    {
        case ExecuteState::NEXT_INST:
            return "NEXT_INST";

        case ExecuteState::LOAD_MEM_REQ:
            return "LOAD_MEM_REQ";

        case ExecuteState::LOAD_MEM_RESP:
            return "LOAD_MEM_RESP";

        case ExecuteState::STORE_MEM_REQ:
            return "STORE_MEM_REQ";

        case ExecuteState::STORE_MEM_RESP:
            return "STORE_MEM_RESP";

        case ExecuteState::MULTIPLE_LOAD_FIRST_MEM_REQ:
            return "MULTIPLE_LOAD_FIRST_MEM_REQ";

        case ExecuteState::MULTIPLE_LOAD_MEM_REQ:
            return "MULTIPLE_LOAD_MEM_REQ";

        case ExecuteState::MULTIPLE_STORE_FIRST_MEM_REQ:
            return "MULTIPLE_STORE_FIRST_MEM_REQ";

        case ExecuteState::MULTIPLE_STORE_MEM_REQ:
            return "MULTIPLE_STORE_MEM_REQ";

        case ExecuteState::FLUSH_PIPELINE:
            return "FLUSH_PIPELINE";

        default:
            return "UNKNOWN";
    }
}

int Execute::executeNextInst()
{
    Reg rd, rt, rdn, rm, rn;
    uint32_t drt, drdn, drm, drn, dxpsr;
    DecodedCondition cond;
    uint32_t rl;
    uint32_t im;
    uint32_t cflag;

    if (decodedInst != nullptr)
    {
        fprintf(stderr,
                "Trying to executeNextInst() with decodedInst != "
                "nullptr\n");
        exit(1);
    }

    decodedInst = decode->getNextInst();
    if (decodedInst == nullptr)
    {
        /* There is no decoded instruction, stall the pipeline */
        DEBUG_CMD(DEBUG_EXECUTE, printf("Execute: stalled, pending decode\n"));

        /* Record that pipeline was stalled because unavailable inst */
        stats->addStallForDecodeCycle();

        return 0;
    }
    else
    {
        DEBUG_CMD(DEBUG_EXECUTE, printf("Execute: new instruction\n"));
    }

    /* Extract all the decoded data for convenience */
    rd = decodedInst->getRegisterNumber(DecodedInstRegIndex::RD);
    rt = decodedInst->getRegisterNumber(DecodedInstRegIndex::RT);
    rdn = decodedInst->getRegisterNumber(DecodedInstRegIndex::RDN);
    rm = decodedInst->getRegisterNumber(DecodedInstRegIndex::RM);
    rn = decodedInst->getRegisterNumber(DecodedInstRegIndex::RN);

    drt = decodedInst->getRegisterData(DecodedInstRegIndex::RT);
    drdn = decodedInst->getRegisterData(DecodedInstRegIndex::RDN);
    drm = decodedInst->getRegisterData(DecodedInstRegIndex::RM);
    drn = decodedInst->getRegisterData(DecodedInstRegIndex::RN);
    dxpsr = decodedInst->getRegisterData(DecodedInstRegIndex::XPSR);

    rl = decodedInst->getRegisterList();

    im = decodedInst->getImmediate();

    cond = decodedInst->getCondition();

    cflag = RegFile::getXpsrC(dxpsr);

    DEBUG_CMD(DEBUG_EXECUTE, printf("Execute:"));

    /* Execute the decoded instruction */
    switch (decodedInst->getOperation())
    {
        /* Multiple memory access instructions */
        case DecodedOperation::POP:
        case DecodedOperation::LDMIA:
            popLdmia(rn, drn, rl);
            break;

        case DecodedOperation::PUSH:
            push(rn, drn, rl);
            break;

        case DecodedOperation::STMIA:
            stmia(rn, drn, rl);
            break;

        /* Memory access instructions */
        case DecodedOperation::STR1:
        case DecodedOperation::STR3:
            str1Str3(rt, drt, rn, drn, im);
            break;

        case DecodedOperation::STR2:
            str2(rt, drt, rn, drn, drm);
            break;

        case DecodedOperation::STRB1:
            strb1(rt, drt, rn, drn, im);
            break;

        case DecodedOperation::STRB2:
            strb2(rt, drt, rn, drn, drm);
            break;

        case DecodedOperation::STRH1:
            strh1(rt, drt, rn, drn, im);
            break;

        case DecodedOperation::STRH2:
            strh2(rt, drt, rn, drn, drm);
            break;

        case DecodedOperation::LDR1:
        case DecodedOperation::LDR4:
            ldr1Ldr4(rt, drn, im);
            break;

        case DecodedOperation::LDR2:
            ldr2(rt, drn, drm);
            break;

        case DecodedOperation::LDR3:
            ldr3(rt, drn, im);
            break;

        case DecodedOperation::LDRB1:
            ldrb1(rt, drn, im);
            break;

        case DecodedOperation::LDRB2:
            ldrb2(rt, drn, drm);
            break;

        case DecodedOperation::LDRH1:
            ldrh1(rt, drn, im);
            break;

        case DecodedOperation::LDRH2:
            ldrh2(rt, drn, drm);
            break;

        case DecodedOperation::LDRSB:
            ldrsb(rt, drn, drm);
            break;

        case DecodedOperation::LDRSH:
            ldrsh(rt, drn, drm);
            break;

        /* Branch instructions */
        case DecodedOperation::B1:
            b1(rm, drm, im, dxpsr, cond);
            break;

        case DecodedOperation::B2:
            b2(rm, drm, im);
            break;

        case DecodedOperation::BL:
            bl(rdn, drdn, im);
            break;

        case DecodedOperation::BLX:
            blx(rdn, drdn, drm);
            break;

        case DecodedOperation::BX:
            bx(rdn, drm);
            break;

        case DecodedOperation::CPY:
            cpy(rd, drm);
            break;

        /* Arithmetic and logic instructions */
        case DecodedOperation::ADC:
            adc(rdn, drdn, drm, cflag);
            break;

        case DecodedOperation::ADD1:
            add1(rd, drn, im);
            break;

        case DecodedOperation::ADD2:
            add2(rdn, drdn, im);
            break;

        case DecodedOperation::ADD3:
            add3(rd, drn, drm);
            break;

        case DecodedOperation::ADD4:
            add4(rdn, drdn, drm);
            break;

        case DecodedOperation::ADD5:
            add5(rd, drm, im);
            break;

        case DecodedOperation::ADD6:
        case DecodedOperation::ADD7:
            add6Add7(rd, drm, im);
            break;

        case DecodedOperation::AND:
            and0(rdn, drdn, drm);
            break;

        case DecodedOperation::ASR1:
            asr1(rd, drm, im);
            break;

        case DecodedOperation::ASR2:
            asr2(rdn, drdn, drm);
            break;

        case DecodedOperation::BIC:
            bic(rdn, drdn, drm);
            break;

        case DecodedOperation::CMN:
            cmn(drn, drm);
            break;

        case DecodedOperation::CMP1:
            cmp1(drn, im);
            break;

        case DecodedOperation::CMP2:
        case DecodedOperation::CMP3:
            cmp2Cmp3(drn, drm);
            break;

        case DecodedOperation::EOR:
            eor(rdn, drdn, drm);
            break;

        case DecodedOperation::LSL1:
            lsl1(rd, drm, im);
            break;

        case DecodedOperation::LSL2:
            lsl2(rdn, drdn, drm);
            break;

        case DecodedOperation::LSR1:
            lsr1(rd, drm, im);
            break;

        case DecodedOperation::LSR2:
            lsr2(rdn, drdn, drm);
            break;

        case DecodedOperation::MOV1:
            mov1(rd, im);
            break;

        case DecodedOperation::MOV2:
            mov2(rd, drm);
            break;

        case DecodedOperation::MUL:
            mul(rdn, drdn, drn);
            break;

        case DecodedOperation::MVN:
            mvn(rd, drm);
            break;

        case DecodedOperation::ORR:
            orr(rdn, drdn, drm);
            break;

        case DecodedOperation::REV:
            rev(rd, drm);
            break;

        case DecodedOperation::REV16:
            rev16(rd, drm);
            break;

        case DecodedOperation::REVSH:
            revsh(rd, drm);
            break;

        case DecodedOperation::ROR:
            ror(rdn, drdn, drm);
            break;

        case DecodedOperation::NEG:
            neg(rd, drn, im);
            break;

        case DecodedOperation::NOP:
            nop();
            break;

        case DecodedOperation::SBC:
            sbc(rdn, drdn, drm, cflag);
            break;

        case DecodedOperation::SUB1:
            sub1(rd, drn, im);
            break;

        case DecodedOperation::SUB2:
            sub2(rdn, drdn, im);
            break;

        case DecodedOperation::SUB3:
            sub3(rd, drm, drn);
            break;

        case DecodedOperation::SUB4:
            sub4(rdn, drdn, im);
            break;

        case DecodedOperation::TST:
            tst(drm, drn);
            break;

        case DecodedOperation::UXTB:
            uxtb(rd, drm);
            break;

        case DecodedOperation::UXTH:
            uxth(rd, drm);
            break;

        case DecodedOperation::SXTB:
            sxtb(rd, drm);
            break;

        case DecodedOperation::SXTH:
            sxth(rd, drm);
            break;

        /* Other instructions */
        case DecodedOperation::BKPT:
            bkpt(im);
            break;

        case DecodedOperation::SVC:
            svc(im);
            break;

        case DecodedOperation::CPS:
            cps(drm);
            break;
    }

    delete decodedInst;
    decodedInst = nullptr;

    return 0;
}
