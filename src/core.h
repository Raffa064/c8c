#ifndef C8C_CORE_H
#define C8C_CORE_H

#include <stddef.h>
#include <stdint.h>

typedef const char *Label;

typedef struct {
  uint8_t N, x, y, n, kk;
  uint16_t nnn:12;
} Opcode;

typedef struct {
  Label label;
  uint16_t addr; // zero = unsolved

  struct {
    Opcode *items;
    size_t count, capacity;
  } array;

  struct { 
    void *prev, *next; 
  } deque;
} Block;

typedef struct {
  Label label;
  Block *block;
  int index;
} Reference;

typedef struct {
  Reference *items;
  size_t count, capacity;
} ReferenceTable;

typedef struct {
  uint8_t *rom;
  size_t rom_size;
} CompilerOutput;

typedef struct {
  Block *head, *tail;
  ReferenceTable rtable;
  CompilerOutput output;
} CompilerState;

Block *create_block(CompilerState *cs, Label label);

CompilerState create_compiler_state();

void push_op(CompilerState *cs, Opcode op);

uint16_t encode_op(Opcode op);

uint8_t high(uint16_t word);

uint8_t low(uint16_t word);

Opcode decode_op(uint16_t inst);

void push_ref(CompilerState *cs, Label label);

void dump_blocks(Block *head);

void dump_reference_table(ReferenceTable rtable);

void dump_output(CompilerState cs);

void dump_compiler_state(CompilerState cs);

void resolve_addresses(CompilerState *cs);

size_t count_opcodes(CompilerState *cs);

void resolve_references(CompilerState *cs);

void generate_binary(CompilerState *cs);

void write_to_file(CompilerState cs, const char *path);

#endif
