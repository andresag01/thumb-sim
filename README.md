# Thumb Timing Simulator

This repository contains a timing approximate simulator of a subset of the Thumb instruction set in the ARMv7-M architecture. It is "approximate" in the sense that I tried to accurately model the timing of each instruction according to the publicly and freely available [documentation](http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.ddi0432c/CHDCICDF.html) for an ARM Cortex-M0 processor. However, I have never seen the actual microarchitecture of the processor and in many cases the behavior simulated by this tool is my "best guess" of what the hardware does.

The simulator implements most instructions in Thumb (**NOT** Thumb-2) with the exception of those related to I/O and multithreading (e.g. WFE, WFI, SEV, etc). Some important facts about the simulator are:

* Runs under both Linux and Mac OS X with GCC and Clang
* There is no I/O, so the CPS instruction was repurposed to output the character in the least signigicant 8 bits of R0
* Technically the Cortex-M0 implements the ARMv6-M architecture, so I only said Cortex-M0 above in the sense that the simulator implements a similar 3 stage pipeline

I use this tool as part of my research in hard real-time hardware garbage collection, so I only add features to the simulator as I need them. I have developed this tool to be easily extensible (to suit the future needs of my project), so there are places in the code with unfinished features or without clear purpose. Also, I have taken shortcuts where possible, for example by not freeing memory when an error occurs or when the simulation terminates.

# Dependencies

There are no dependencies on anything other than the compiler, GNU Make and a libc. To build, simply type:

```
make
```

# Running a Program

The simulator can run programs compiled with either arm-none-eabi-gcc or clang. I have included a simple example that prints "hello world!" using arm-none-eabi-gcc. It can be compiled and run using the following commands:

```
make -C example
./simulator -b example/example.bin
```

# Notes

**I am sharing it for anyone who wants to use it for whatever purpose. However, there are no guarantees that the simulator is bug free and there are no warranties with this software, so use it at your own risk.**

Please let me know if you find any bugs (by raising an issue or emailing me), such as inaccuracies in the timing of an operation.
