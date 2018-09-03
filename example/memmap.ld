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
MEMORY
{
    ram(WAIL) : ORIGIN = 0x00000000, LENGTH = 64K
}

ENTRY(_start)

/* Set the minimum heap size */
_min_heap_size = 15K;
/* Set the minimum stack size */
_min_stack_size = 10K;

_ram_start = ORIGIN(ram);
_ram_end = ORIGIN(ram) + LENGTH(ram);

_stack_end = ORIGIN(ram) + LENGTH(ram);

SECTIONS
{
    /* Program code */
    .text :
    {
        . = ALIGN(4);
        _text_start = .;

        *(.text*)

        . = ALIGN(4);
        _text_end = .;
    } > ram

    /* Initialised data */
    .data :
    {
        . = ALIGN(4);
        _data_start = .;

        *(.data*)

        . = ALIGN(4);
        _data_end = .;
    } > ram

    /* Uninitialised data */
    .bss :
    {
        . = ALIGN(4);
        _bss_start = .;

        *(.bss*)

        . = ALIGN(4);
        _bss_end = .;
    } > ram

    /* Read only data */
    .rodata : {
        . = ALIGN(4);

        *(.rodata*)

        . = ALIGN(4);
    } > ram

    /*
     * The heap and stack sections are just checks to ensure there is enough
     * space
     */
    /* Heap */
    .heap :
    {
        . = ALIGN(4);
        _heap_start = .;

        . = . + _min_heap_size;

        . = ALIGN(4);
        _heap_end = .;
    } > ram

    /* Stack */
    .stack :
    {
        . = ALIGN(4);
        _stack_start = .;

        . = . + _min_stack_size;

        . = ALIGN(4);
    } > ram
}
