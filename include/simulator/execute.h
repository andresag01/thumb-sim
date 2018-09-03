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
#ifndef _EXECUTE_H_
#define _EXECUTE_H_

#include "simulator/decode.h"
#include "simulator/fetch.h"
#include "simulator/memory.h"
#include "simulator/regfile.h"
#include "simulator/stats.h"
#include "simulator/utils.h"

#include <list>

enum class ExecuteState
{
    NEXT_INST,
    LOAD_MEM_REQ,
    LOAD_MEM_RESP,
    STORE_MEM_REQ,
    STORE_MEM_RESP,
    MULTIPLE_LOAD_FIRST_MEM_REQ,
    MULTIPLE_LOAD_MEM_REQ,
    MULTIPLE_STORE_FIRST_MEM_REQ,
    MULTIPLE_STORE_MEM_REQ,
    FLUSH_PIPELINE,
};

enum class MemoryInstructionType
{
    SBYTE,
    UBYTE,
    SHALFWORD,
    UHALFWORD,
    WORD,
};

class Execute
{
public:
    Execute(Fetch *fetchIn,
            Decode *decodeIn,
            RegFile *regFileIn,
            Memory *memIn,
            Statistics *statsIn);
    void flushPipeline();
    int run();

    bool isStalled();

    static std::string execStateToStr(ExecuteState state);

private:
    /* Execution state machine functions */
    /* Execute next decoded instruction */
    int executeNextInst();
    /* Single memory load */
    int executeLoadMemReq();
    int executeLoadMemResp();
    /* Single memory store */
    int executeStoreMemReq();
    int executeStoreMemResp();
    /* Multiple memory load */
    int executeMultipleLoadFirstMemReq();
    int executeMultipleLoadMemReq();
    /* Multiple memory store */
    int executeMultipleStoreFirstMemReq();
    int executeMultipleStoreMemReq();
    /* Convenient function to flush the pipeline with 1 cycle delay */
    int executeFlushPipeline();

    /* Multiple memory access instructions */
    int popLdmia(Reg rn, uint32_t drn, uint32_t rl);
    int stmia(Reg rn, uint32_t drn, uint32_t rl);
    int push(Reg rn, uint32_t drn, uint32_t rl);

    /* Memory access instructions */
    int ldr1Ldr4(Reg rt, uint32_t drn, uint32_t im);
    int ldr2(Reg rt, uint32_t drn, uint32_t drm);
    int ldr3(Reg rt, uint32_t drn, uint32_t im);
    int ldrb1(Reg rt, uint32_t drn, uint32_t im);
    int ldrb2(Reg rt, uint32_t drn, uint32_t drm);
    int ldrh1(Reg rt, uint32_t drn, uint32_t im);
    int ldrh2(Reg rt, uint32_t drn, uint32_t drm);
    int ldrsb(Reg rt, uint32_t drn, uint32_t drm);
    int ldrsh(Reg rt, uint32_t drn, uint32_t drm);
    int str1Str3(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t im);
    int str2(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t drm);
    int strb1(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t im);
    int strb2(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t drm);
    int strh1(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t im);
    int strh2(Reg rt, uint32_t drt, Reg rn, uint32_t drn, uint32_t drm);

    /* Branch instructions */
    int b1(Reg rm,
           uint32_t drm,
           uint32_t im,
           uint32_t dxpsr,
           DecodedCondition cond);
    int b2(Reg rm, uint32_t drm, uint32_t im);
    int bl(Reg rdn, uint32_t drdn, uint32_t im);
    int blx(Reg rdn, uint32_t drdn, uint32_t drm);
    int bx(Reg rdn, uint32_t drm);

    /* Arithmetic and logic instructions */
    int adc(Reg rdn, uint32_t drdn, uint32_t drm, uint32_t cflag);
    int add1(Reg rd, uint32_t drn, uint32_t im);
    int add2(Reg rdn, uint32_t drdn, uint32_t im);
    int add3(Reg rd, uint32_t drn, uint32_t drm);
    int add4(Reg rdn, uint32_t drdn, uint32_t drm);
    int add6Add7(Reg rd, uint32_t drm, uint32_t im);
    int add5(Reg rd, uint32_t drm, uint32_t im);
    int and0(Reg rdn, uint32_t drdn, uint32_t drm);
    int asr1(Reg rd, uint32_t drm, uint32_t im);
    int asr2(Reg rdn, uint32_t drdn, uint32_t drm);
    int bic(Reg rdn, uint32_t drdn, uint32_t drm);
    int cmn(uint32_t drn, uint32_t drm);
    int cmp1(uint32_t drn, uint32_t im);
    int cmp2Cmp3(uint32_t drn, uint32_t drm);
    int cpy(Reg rd, uint32_t drm);
    int eor(Reg rdn, uint32_t drdn, uint32_t drm);
    int lsl1(Reg rd, uint32_t drm, uint32_t im);
    int lsl2(Reg rdn, uint32_t drdn, uint32_t drm);
    int lsr1(Reg rd, uint32_t drm, uint32_t im);
    int lsr2(Reg rdn, uint32_t drdn, uint32_t drm);
    int mov1(Reg rd, uint32_t im);
    int mov2(Reg rd, uint32_t drm);
    int mul(Reg rdn, uint32_t drdn, uint32_t drn);
    int mvn(Reg rd, uint32_t drm);
    int neg(Reg rd, uint32_t drn, uint32_t im);
    int nop();
    int orr(Reg rdn, uint32_t drdn, uint32_t drm);
    int rev(Reg rd, uint32_t drm);
    int rev16(Reg rd, uint32_t drm);
    int revsh(Reg rd, uint32_t drm);
    int ror(Reg rdn, uint32_t drdn, uint32_t drm);
    int sbc(Reg rdn, uint32_t drdn, uint32_t drm, uint32_t cflag);
    int sub1(Reg rd, uint32_t drn, uint32_t im);
    int sub2(Reg rdn, uint32_t drdn, uint32_t im);
    int sub3(Reg rd, uint32_t drm, uint32_t drn);
    int sub4(Reg rdn, uint32_t drdn, uint32_t im);
    int tst(uint32_t drm, uint32_t drn);
    int uxtb(Reg rd, uint32_t drm);
    int uxth(Reg rd, uint32_t drm);
    int sxtb(Reg rd, uint32_t drm);
    int sxth(Reg rd, uint32_t drm);

    /* Miscellaneous instructions */
    int bkpt(uint32_t im);
    int svc(uint32_t im);
    int cps(uint32_t drm);

    /* Conditional flags handling */
    bool checkCondition(DecodedCondition cond, uint32_t xpsr);
    void calculateXpsrZ(uint32_t res);
    void calculateXpsrN(uint32_t res, uint32_t bits);
    void calculateXpsrQ();
    void calculateXpsrFlags(uint32_t res,
                            uint32_t op0,
                            uint32_t op1,
                            uint32_t cflag,
                            uint32_t bits);
    void calculateXpsrC(uint32_t res,
                        uint32_t op0,
                        uint32_t op1,
                        uint32_t cflag,
                        uint32_t bits);

    /* Helper functions */
    int ldr(Reg rt, uint32_t drn, uint32_t offset, MemoryInstructionType type);
    int str(Reg rt,
            uint32_t drt,
            Reg rn,
            uint32_t drn,
            uint32_t offset,
            MemoryInstructionType type);
    void populateRegisterList(std::list<Reg> &regList, uint32_t rl);
    int requestNextStore();

    /* Helper load and store formatting functions */
    void formatDataForMemLoad(MemoryInstructionType type,
                              uint32_t &data,
                              uint32_t offset);
    void formatDataForMemStore(MemoryInstructionType type,
                               uint32_t data,
                               uint32_t &drt,
                               uint32_t offset);

    /* Calculate stats */
    void calculateExecCycles();

    /* State of the execution unit */
    ExecuteState execState{ ExecuteState::NEXT_INST };
    /*
     * The GC is simulated after the execution unit, so we must keep a copy
     * of the state that is currently executed in the cycle where the GC
     * calls isInIntermediateState()
     */
    ExecuteState curExecState{ ExecuteState::NEXT_INST };

    /* Temporary data for the execute state machine */
    struct LoadTemporaries
    {
        uint32_t ptr;
        uint32_t byteOffset;
        MemoryInstructionType type;
        Reg destReg;
        uint32_t memToken;
        uint32_t data;
    } loadTmps;
    struct StoreTemporaries
    {
        uint32_t ptr;
        uint32_t byteOffset;
        MemoryInstructionType type;
        uint32_t memToken;
        uint32_t data;
        Reg dataReg;
        Reg addrReg;
    } storeTmps;
    struct MultipleLoadTemporaries
    {
        uint32_t ptr;
        uint32_t byteOffset;
        std::list<Reg> regList;
        uint32_t memToken;
        uint32_t data;
        Reg destReg;
        Reg baseReg;
    } mloadTmps;
    struct MultipleStoreTemporaries
    {
        uint32_t ptr;
        std::list<Reg> regList;
        uint32_t byteOffset;
        uint32_t data;
        uint32_t memToken;
        Reg baseReg;
        Reg srcReg;
        DecodedOperation op;
    } mstoreTmps;

    DecodedInst *decodedInst{ nullptr };

    RegFile *regFile{ nullptr };
    Decode *decode{ nullptr };
    Fetch *fetch{ nullptr };
    Memory *mem{ nullptr };
    Statistics *stats{ nullptr };
};

#endif /* _EXECUTE_H_ */
