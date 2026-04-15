#ifndef C8C_CORE_H
#define C8C_CORE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

// Labels are strdupped from source code, so they're heap allocated
typedef char *Label;

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
  struct {
    const char **items;
    size_t count, capacity;
  } input_files;

  struct {
    bool main_jump; // Appends a jump for main label (or whatever is set as entrypoint)
    const char *entrypoint; // NOTE: it's stacked allocated from CLI arguments!
  } opt;
} CompilerInput;

typedef struct {
  uint8_t *rom;
  size_t rom_size;
} CompilerOutput;

typedef struct {
  CompilerInput input;
  CompilerOutput output;
  Block *head, *tail;
  ReferenceTable rtable;
} CompilerState;

Block *create_block(CompilerState *cs, Label label);

CompilerState create_compiler_state(CompilerInput input);

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

void parse_input_files(CompilerState *cs); 

void resolve_addresses(CompilerState *cs);

size_t count_opcodes(CompilerState *cs);

void resolve_references(CompilerState *cs);

void generate_binary(CompilerState *cs);

void write_to_file(CompilerState cs, const char *path);

void free_compiler_state(CompilerState *cs);

#endif
