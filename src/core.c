#include "core.h"
#include "macros.h"
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

Block *create_block(CompilerState *cs, Label label) {
  Block *block = malloc(sizeof(Block));
  memset(block, 0, sizeof(Block));

  block->label = label;

  if (cs) {
    block->deque.prev = cs->tail;
    cs->tail->deque.next = block;
    cs->tail = block;
  }

  return block;
}

CompilerState create_compiler_state(CompilerInput input) {
  Block *dummy = create_block(NULL, NULL);

  CompilerState cs = {
    .input = input,
    .head = dummy,
    .tail = dummy,
    .rtable = {0}
  };

  // Push jump instruction for entry point (usefull for unordered input files)
  if (input.opt.main_jump) {
    // JP @main
    push_op(&cs, OPCODE(1000)); 
    push_ref(&cs, cs.input.opt.entrypoint);
  }

  return cs;
}

void push_op(CompilerState *cs, Opcode op) {
  da_append(cs->tail->array, op);
}

uint16_t encode_op(Opcode op) {
  uint16_t inst = 0;

  inst |= (op.N & 0xF) << 12;
  inst |= (op.x & 0xF) << 8;
  inst |= (op.y & 0xF) << 4;
  inst |= op.n & 0xF;
  inst |= op.kk & 0xFF;
  inst |= op.nnn & 0xFFF;

  return inst;
}

uint8_t high(uint16_t word) {
  return word >> 8;
}

uint8_t low(uint16_t word) {
  return word & 0xFF;
}

Opcode decode_op(uint16_t inst) {
  return (Opcode) {
    .N = (inst >> 12) & 0xF,
    .x = (inst >> 8) & 0xF,
    .y = (inst >> 4) & 0xF,
    .n = inst & 0xF,
    .kk = inst & 0xFF,
    .nnn = inst & 0xFFF
  };
}

void push_ref(CompilerState *cs, Label label) {
  Reference ref = {
    .block = cs->tail,
    .index = (cs->tail->array.count - 1),
    .label = label
  };
  
  da_append(cs->rtable, ref);
}

void dump_blocks(Block *head) {
  Block *curr = head;

  printf("Basic Blocks:\n");
  while (curr) {
    if (curr->addr == 0) {
      printf("  $---- %s\n", curr->label);
    } else {
      printf("  $%04x %s\n", curr->addr, curr->label);
    }

    for (int i = 0; i < curr->array.count; ++i) {
      Opcode op = curr->array.items[i];
      printf("  %5d %04x\n", i, encode_op(op));
    }
    
    curr = curr->deque.next;
  }
}

void dump_reference_table(ReferenceTable rtable) {
  printf("Reference Table:\n");
  for (int i = 0; i < rtable.count; ++i) {
    Reference ref = rtable.items[i];
    printf("  %s-%d : %s\n", ref.block->label, ref.index, ref.label);
  }
}

void dump_output(CompilerState cs) {
  printf("ROM:\n");

  uint16_t last = 256;
  size_t count = 0;
  for (size_t i = 0; i < cs.output.rom_size; ++i) {
    uint8_t byte = cs.output.rom[i];

    if (byte == last)
      count++;
    else {
      if (count > 1)
        printf(".. ");

      printf("%02x ", byte);
      count = 1;
    }

    last = byte;
  }

  if (count > 1)
    printf("...");

  printf("\n");
}

void dump_compiler_state(CompilerState cs) {
  dump_blocks(cs.head);
  dump_reference_table(cs.rtable);

  if (cs.output.rom)
    dump_output(cs);
}

void parse_input_files(CompilerState *cs) {
  for (int i = 0; i < cs->input.input_files.count; ++i) {
    const char * input_path = cs->input.input_files.items[i];
    parse_file(cs, input_path);
  }
}

void resolve_addresses(CompilerState *cs) {
  uint16_t addr = PROGRAMS_START_ADDR;
  
  Block *curr = cs->head;
  while (curr) {
    curr->addr = addr;
    addr += curr->array.count * OPCODE_SIZE_BYTES;
    curr = curr->deque.next;
  }
}

size_t count_opcodes(CompilerState *cs) {
  size_t count = 0;

  Block *curr = cs->head;
  while (curr) {
    count += curr->array.count;
    curr = curr->deque.next;
  }

  return count;
}

void resolve_references(CompilerState *cs) {
  for (int i = 0; i < cs->rtable.count; ++i) {
    Reference ref = cs->rtable.items[i];

    uint8_t found = 0;
    Block *curr = cs->head;
    while (curr) {
      if (curr->label && strcmp(curr->label, ref.label) == 0) {
        found = 1;
        Opcode *op = &ref.block->array.items[ref.index];
        op->nnn = curr->addr;
        break;
      }

      curr = curr->deque.next;
    }

    if (!found) {
      fprintf(stderr, "Unresolved reference: '%s'\n", ref.label);
      exit(1);
    }
  }
}

void generate_binary(CompilerState *cs) {
  size_t op_count = count_opcodes(cs);
  size_t rom_size = op_count * OPCODE_SIZE_BYTES;
  uint8_t *rom = malloc(rom_size);
  memset(rom, 0, rom_size);

  Block *curr = cs->head;
  while (curr) {
    for (int i = 0; i < curr->array.count; i++) {
      uint16_t addr = (curr->addr + i * 2) - PROGRAMS_START_ADDR;
      Opcode op = curr->array.items[i];
      uint16_t inst = encode_op(op);
      
      rom[addr] = high(inst); 
      rom[addr + 1] = low(inst);
    }

    curr = curr->deque.next;
  }

  cs->output.rom = rom;
  cs->output.rom_size = rom_size;
}

void write_to_file(CompilerState cs, const char *path) {
  FILE *rom_file = fopen(path, "w");

  if (!rom_file) {
    fprintf(stderr, "Can't write to file: %s\n", path);
    exit(1);
  }

  size_t total = 0;
  while (total < cs.output.rom_size) {
    size_t bytes = fwrite(cs.output.rom + total, 1, cs.output.rom_size - total, rom_file);
    if (bytes == 0) break;
    total += bytes;
  }

  fclose(rom_file);
}
