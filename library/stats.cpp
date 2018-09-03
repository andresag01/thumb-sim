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
#include "simulator/stats.h"

#include "simulator/utils.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <string>
#include <unordered_map>

#define MAKE_INC_FUNCTION(func_name, member) \
    void Statistics::add##func_name()        \
    {                                        \
        member++;                            \
    }

MAKE_INC_FUNCTION(Cycle, cycles)
MAKE_INC_FUNCTION(FetchCycle, fetchMemCycles)
MAKE_INC_FUNCTION(ExecuteCycle, executeMemCycles)

MAKE_INC_FUNCTION(StallForDecodeCycle, stalledForDecodeCycles)

MAKE_INC_FUNCTION(BranchTaken, branchTaken)
MAKE_INC_FUNCTION(BranchNotTaken, branchNotTaken)

#define MAKE_SET_FUNCTION(func_name, member, type) \
    void Statistics::set##func_name(type size)     \
    {                                              \
        member = size;                             \
    }

MAKE_SET_FUNCTION(ProgramSizeBytes, programSizeBytes, uint32_t)
MAKE_SET_FUNCTION(MemSizeWords, memSizeWords, uint32_t)
MAKE_SET_FUNCTION(MemAccessWidthWords, memAccessWidthWords, uint32_t)

#define MAKE_ADD_TO_UMAP_FUNCTION(func_name, member, type) \
    void Statistics::add##func_name(type key)              \
    {                                                      \
        const auto &iter = member.find(key);               \
                                                           \
        if (iter == member.end())                          \
        {                                                  \
            /* Object size not found */                    \
            member.insert({ key, 1 });                     \
        }                                                  \
        else                                               \
        {                                                  \
            /* Object size already in the map */           \
            iter->second = iter->second + 1;               \
        }                                                  \
    }
MAKE_ADD_TO_UMAP_FUNCTION(Instruction, instCount, Instruction)

std::string Statistics::getInstructionStr(Instruction inst)
{
    switch (inst)
    {
        case Instruction::ADC:
            return "adc";

        case Instruction::ADD:
            return "add";

        case Instruction::AND:
            return "and";

        case Instruction::ASR:
            return "asr";

        case Instruction::B:
            return "b";

        case Instruction::BIC:
            return "bic";

        case Instruction::BL:
            return "bl";

        case Instruction::BLX:
            return "blx";

        case Instruction::BX:
            return "bx";

        case Instruction::CMN:
            return "cmn";

        case Instruction::CMP:
            return "cmp";

        case Instruction::EOR:
            return "eor";

        case Instruction::LDMIA:
            return "ldmia";

        case Instruction::LDR:
            return "ldr";

        case Instruction::LDRB:
            return "ldrb";

        case Instruction::LDRH:
            return "ldrh";

        case Instruction::LDRSB:
            return "ldrsb";

        case Instruction::LDRSH:
            return "ldrsh";

        case Instruction::LSL:
            return "lsl";

        case Instruction::LSR:
            return "lsr";

        case Instruction::MVN:
            return "mvn";

        case Instruction::MOV:
            return "mov";

        case Instruction::MUL:
            return "mul";

        case Instruction::NEG:
            return "neg";

        case Instruction::NOP:
            return "nop";

        case Instruction::ORR:
            return "orr";

        case Instruction::REV:
            return "rev";

        case Instruction::REV16:
            return "rev16";

        case Instruction::REVSH:
            return "revsh";

        case Instruction::ROR:
            return "ror";

        case Instruction::SBC:
            return "sbc";

        case Instruction::PUSH:
            return "push";

        case Instruction::STMIA:
            return "stmia";

        case Instruction::STR:
            return "str";

        case Instruction::STRB:
            return "strb";

        case Instruction::STRH:
            return "strh";

        case Instruction::SUB:
            return "sub";

        case Instruction::SXTB:
            return "sxtb";

        case Instruction::SXTH:
            return "sxth";

        case Instruction::TST:
            return "tst";

        case Instruction::UXTB:
            return "uxtb";

        case Instruction::UXTH:
            return "uxth";

        default:
            return "unknown";
    }
}

void Statistics::print()
{
    uint64_t unusedMemCycles = cycles - executeMemCycles;
    std::string prefix = "    ";

    /* Sanity checks */
    if (cycles < executeMemCycles)
    {
        fprintf(stderr,
                "Total cycles is less than sum of individual "
                "components\n");
        exit(1);
    }

    printf("== Simulation statistics ==\n");

    printf("System configuration:\n");
    printf("%sMemory size: %" PRIu32 " bytes (%" PRIu32 " words)\n",
           prefix.c_str(),
           WORD_TO_BYTE_SIZE(memSizeWords),
           memSizeWords);
    printf("%sMemory access width: %" PRIu32 " bytes (%" PRIu32 " words)\n",
           prefix.c_str(),
           WORD_TO_BYTE_SIZE(memAccessWidthWords),
           memAccessWidthWords);

    printf("\n");

    printf("General information:\n");
    printf("%sTotal cycles: %" PRIu64 "\n", prefix.c_str(), cycles);
    printf("%sFetch cycles: %" PRIu64 " %%%f\n",
           prefix.c_str(),
           fetchMemCycles,
           100.0f * ((float)fetchMemCycles / (float)cycles));
    printf("%sExecute cycles: %" PRIu64 " %%%f\n",
           prefix.c_str(),
           executeMemCycles,
           100.0f * ((float)executeMemCycles / (float)cycles));
    printf("%sUnused cycles: %" PRIu64 " %%%f\n",
           prefix.c_str(),
           unusedMemCycles,
           100.0f * ((float)unusedMemCycles / (float)cycles));

    printf("\n");

    printf("Stalling information:\n");
    printf("%sStalled for decode cycles: %" PRIu64 " %%%f\n",
           prefix.c_str(),
           stalledForDecodeCycles,
           100.0f * ((float)stalledForDecodeCycles / (float)cycles));

    printf("\n");

    printf("Garbage collection\n");
    printf("%sProgram memory: %" PRIu32 " bytes (%" PRIu32 " words)\n",
           prefix.c_str(),
           programSizeBytes,
           BYTE_TO_WORD_SIZE(programSizeBytes));

    printf("\n");

    printf("Instruction execution:\n");
    uint64_t totalInst = 0;
    uint64_t branches = 0;
    uint64_t stores = 0;
    uint64_t loads = 0;
    uint64_t other = 0;
    for (auto iter = instCount.begin(); iter != instCount.end(); ++iter)
    {
        printf("%s%-6s %" PRIu64 "\n",
               prefix.c_str(),
               Statistics::getInstructionStr(iter->first).c_str(),
               iter->second);

        totalInst = totalInst + iter->second;

        /* Classify instructions by type */
        switch (iter->first)
        {
            case Instruction::B:
            case Instruction::BL:
            case Instruction::BLX:
            case Instruction::BX:
                branches = branches + iter->second;
                break;

            case Instruction::LDMIA:
            case Instruction::LDR:
            case Instruction::LDRB:
            case Instruction::LDRH:
            case Instruction::LDRSB:
            case Instruction::LDRSH:
                loads = loads + iter->second;
                break;

            case Instruction::PUSH:
            case Instruction::STMIA:
            case Instruction::STR:
            case Instruction::STRB:
            case Instruction::STRH:
                stores = stores + iter->second;
                break;

            default:
                other = other + iter->second;
                break;
        }
    }

    printf("\n");

    /*
     * We can branch with dedicated branch instructions, add, mov and pop. In
     * the case of add and mov instructions these are counted as branches, but
     * for pop this is not the case, so branchTaken + branchNotTaken will not
     * be exactly as the value in branches
     */
    if (branches + loads < branchTaken + branchNotTaken)
    {
        fprintf(stderr, "Branching information does not match\n");
        exit(1);
    }
    else
    {
        branches = branchTaken + branchNotTaken;
    }

    printf("%s%-7s %" PRIu64 " %%%f\n",
           prefix.c_str(),
           "Branch",
           branches,
           100.0f * ((float)branches / (float)totalInst));
    printf("%s%s%-18s %" PRIu64 " %%%f\n",
           prefix.c_str(),
           prefix.c_str(),
           "Branch taken",
           branchTaken,
           100.0f * ((float)branchTaken / (float)branches));
    printf("%s%s%-18s %" PRIu64 " %%%f\n",
           prefix.c_str(),
           prefix.c_str(),
           "Branch not taken",
           branchNotTaken,
           100.0f * ((float)branchNotTaken / (float)branches));
    printf("%s%-7s %" PRIu64 " %%%f\n",
           prefix.c_str(),
           "Load",
           loads,
           100.0f * ((float)loads / (float)totalInst));
    printf("%s%-7s %" PRIu64 " %%%f\n",
           prefix.c_str(),
           "Store",
           stores,
           100.0f * ((float)stores / (float)totalInst));
    printf("%s%-7s %" PRIu64 " %%%f\n",
           prefix.c_str(),
           "Other",
           other,
           100.0f * ((float)other / (float)totalInst));
    printf("%s%-7s %" PRIu64 "\n", prefix.c_str(), "Total", totalInst);
}
