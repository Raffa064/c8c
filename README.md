# C8C - Chip8 Compiler

This project implements a basic compiler for [Chip8](https://en.wikipedia.org/wiki/CHIP-8) following [CowGod's technical reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM).

## The Compiler

This compiler follows a simple pipeline.

First, the source code is tokenized using the
[lex.h](https://github.com/Raffa064/lex.h) library, converting the input into tokens.

These tokens are then matched against parsing rules to produce a list of `opcodes`.  
An `opcode` represents a single instruction.

During parsing, the compiler also builds two additional structures: **basic blocks** and **references**.

- **Basic Blocks** are sequences of instructions defined by labels.  
  Each block contains a list of opcodes and links to neighboring blocks, forming a linked structure.

- **References** are created when an instruction targets a label (i.e., a block address).  
  Since target addresses are not known during parsing, these references are deferred.

After parsing the entire source, the compiler has:
- a list of blocks (with their opcodes)
- a list of unresolved references

The next step is **reference resolution**.  
Each block is assigned a concrete memory address, and all deferred references are updated accordingly.

Finally, the opcodes are encoded into binary form.  
Each instruction is 2 bytes, and the result is written sequentially into an output buffer.

Done.

## About Chip8 Assembly

Chip8 has 16 general porpose registers (from V0 to VF), and a 16bit address register (I) used to point memory locations.

It can only add and subtract numbers, using instructions like ADD and SUB. But you can also use shift operations (SHL/SHR) for mutiplying or dividing by 2.

After set to any value, Delay Timer (DT) and Sound Timer (ST) are updated at 60Hz freq, turning all the way back to zero.

You can use JP to move freely over the memory, but take care to not move to on executable memory regions, otherwise you can't predict how the system will behave.

You can also jump over "subroutines" using CALL instruction, but Chip8 has a really small stack tho, with ony 16 slots. Overflowing it may lead to UB or errors depending on the Emulator implementation.

In this compiler you can define labels to name certain pieces of codes. It can be used to mark subroutines and loops, like the following example:

```asm
LD v0, $A          ; v0 is set to 10 (hex)
loop:
  SNE v0, $0       ; Check if v0 is not equal to zero and skip next instruction if so
    JP @end_loop   ; End loop
    
  ADD v0, $FF      ; Subtract one from v0
  JP @loop         ; Go back to loop
end_loop:
    ; end of the execution
```

> [!TIP]
The compiler syntax is almost the same used in the [tecnical reference](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM).

## Running compiled ROMs

To run the compiled ROMs, you will need a CHIP-8 emulator. You can use my emulator [see repo](https://github.com/Raffa064/c8-emu).
