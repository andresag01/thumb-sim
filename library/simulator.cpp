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
#include "simulator/simulator.h"

#include "simulator/config.h"
#include "simulator/debug.h"
#include "simulator/processor.h"

#include <cinttypes>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>

class CmdLineArgs
{
public:
    char *bin{ nullptr };
    uint32_t memSizeWords{ MEM_SIZE_WORDS };
    uint32_t memAccessWidthWords{ MEM_ACCESS_WIDTH_WORDS };

    static constexpr const char *HELP_MSG =
        "Thumb timing simulator.\n"
        "\n"
        "USAGE: %s -b <file> [-m <val> | -w <val> | -h]\n"
        "\n"
        "  -m    Memory size (words). Default: %" PRIu32 "\n"
        "  -w    Memory access width (words). Default: %" PRIu32 "\n"
        "  -b    Program binary file\n"
        "  -h    Prints this help message\n";
};

int Simulator::run(char *programBinFile)
{
    return run(programBinFile, MEM_SIZE_WORDS, MEM_ACCESS_WIDTH_WORDS);
}

int Simulator::run(char *programBinFile,
                   uint32_t memSizeWordsIn,
                   uint32_t memAccessWidthWordsIn)
{
    int ret;
    uint32_t cycle = 0;

    proc = new Processor(memSizeWordsIn, memAccessWidthWordsIn);

    /* Avoid compiler warnings when not debugging */
    (void)cycle;

    if ((ret = proc->reset(programBinFile)) != 0)
    {
        fprintf(stderr, "Failed to reset processor (%d)\n", ret);
        return ret;
    }

    do
    {
        DEBUG_CMD(DEBUG_ALL, printf("== cycle %" PRIu32 " ==\n", cycle++));
    } while (proc->simulateCycle() == 0);

    delete proc;

    return 0;
}

int main(int argc, char **argv)
{
    Simulator sim;
    CmdLineArgs args;
    int i;
    int converted;

    /* Parse command line arguments */
    for (i = 1; i < argc; i++)
    {
        if (strcmp(argv[i], "-h") == 0)
        {
            printf(CmdLineArgs::HELP_MSG,
                   argv[0],
                   MEM_SIZE_WORDS,
                   MEM_ACCESS_WIDTH_WORDS);
            return EXIT_SUCCESS;
        }
        else if (strcmp(argv[i], "-m") == 0)
        {
            i++;
            if (i >= argc)
            {
                /* Ran out of arguments, fail */
                fprintf(stderr, "Option -m requires an argument\n");
                return EXIT_FAILURE;
            }

            converted = atoi(argv[i]);
            if (converted <= 0)
            {
                fprintf(stderr, "Invalid value %s for -m\n", argv[i]);
                return EXIT_FAILURE;
            }
            else
            {
                args.memSizeWords = static_cast<uint32_t>(converted);
            }
        }
        else if (strcmp(argv[i], "-w") == 0)
        {
            i++;
            if (i >= argc)
            {
                /* Ran out of arguments, fail */
                fprintf(stderr, "Option -w requires an argument\n");
                return EXIT_FAILURE;
            }

            converted = atoi(argv[i]);
            if (converted <= 0)
            {
                fprintf(stderr, "Invalid value %s for -w\n", argv[i]);
                return EXIT_FAILURE;
            }
            else
            {
                args.memAccessWidthWords = static_cast<uint32_t>(converted);
            }
        }
        else if (strcmp(argv[i], "-b") == 0)
        {
            i++;
            if (i >= argc)
            {
                /* Ran out of arguments, fail */
                fprintf(stderr, "Option -b requires an argument\n");
                return 1;
            }
            args.bin = argv[i];
        }
        else
        {
            fprintf(stderr, "Unrecognized option '%s'\n", argv[i]);
            exit(1);
        }
    }

    /* Print full command for reference */
    for (i = 0; i < argc; i++)
    {
        printf("%s%c", argv[i], (i + 1 == argc) ? '\n' : ' ');
    }

    if (args.bin == nullptr)
    {
        fprintf(stderr, "A program binary is needed to run the simulator\n");
        return EXIT_FAILURE;
    }

    if (sim.run(args.bin, args.memSizeWords, args.memAccessWidthWords) != 0)
    {
        return EXIT_FAILURE;
    }
    else
    {
        return EXIT_SUCCESS;
    }
}
