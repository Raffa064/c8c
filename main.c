#include <assert.h>
#include <ctype.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEX_IMPLEMENTATION
#define LEX_USE_XMACRO
#include "lex.h"

/// MACROS

#define PROGRAMS_START_ADDR 0x200
#define OPCODE(op) decode_op(0x##op)
#define OPCODE_SIZE_BYTES 2
#define HEX_CHARS "0123456789abcdefABCDEF"
#define ARRAYLEN(a) (sizeof(a)/sizeof(a[0]))
#define DA_DEFAULT_CAPACITY 8

#define da_append(da, item) \
do { \
  if ((da).capacity < (da).count + 1) { \
    if ((da).capacity == 0) \
      (da).capacity = DA_DEFAULT_CAPACITY; \
    else \
      (da).capacity += (da).capacity / 2; \
    (da).items = realloc((da).items, (da).capacity * sizeof((da).items[0])); \
  } \
  (da).items[(da).count++] = (item); \
} while(0);


/// TYPEDEFS

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

// COMPILER STATE

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

CompilerState create_compiler_state() {
  Block *dummy = create_block(NULL, NULL);

  return (CompilerState) {
    .head = dummy,
    .tail = dummy,
    .rtable = {0}
  };
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


#define C8C(X)\
  X(WS,      lex_builtin_rule_ws,              .skip = true)\
  X(COMMENT, lex_builtin_rule_asmlike_comment, .skip = true)\
  X(KEYWORD, c8c_rule_keyword)\
  X(TERM,    c8c_rule_term)\
  X(HEX,     c8c_rule_hex)\
  X(ID,      lex_builtin_rule_id)

#define _X_impl(_, rule, ...) size_t rule(LexCursor cursor);
C8C(_X_impl)

LEX_ENUMX(C8C);
LexType c8c_types[C8C_COUNT] = LEX_TYPEX(C8C);

/// PARSE FUNCTIONS

// label:
bool parse_label_def(Lex *l, Label *out) {
  Lex b = LEX_BRANCH(l);

  LexToken tk;
  if (lex_consume(&b, &tk, ID))
    if (lex_skip(&b, TERM, ":")) {
        *out = lex_tkstr_dup(tk);

        LEX_MERGE_BRANCH(l, b);
        return true;
    }

  return false;
}

// @label
bool parse_label_ref(Lex *l, Label *out) {
  Lex b = LEX_BRANCH(l);

  if (lex_skip(&b, TERM, "@")) {
    LexToken tk;
    if (lex_consume(&b, &tk, ID)) {
      *out = lex_tkstr_dup(tk);

        LEX_MERGE_BRANCH(l, b);
        return true;
    }
  }

  return false;
}

// $200
bool parse_hex(Lex *l, uint16_t *out) {
  LexToken tk;
  if (lex_consume(l, &tk, HEX)) {
    *out = strtol(tk.sv.begin + 1, NULL, 16);
    return true;
  } 


  return false;
}

// v0
bool parse_v_reg(Lex *l, uint8_t *out) {
  Lex b = LEX_BRANCH(l);

  LexToken tk;
  if (lex_consume(&b, &tk, ID)) {
    LexStringView sv = tk.sv;

    // A v-register consists of 2 characteres: V + number (Ex: v0) 
    if (lex_view_count(sv) != 2)   return false; 
    if (tolower(sv.begin[0]) != 'v') return false;

    char n = tolower(sv.begin[1]);
    const char *hexchars = "0123456789abcdef";
    char *ptr = strchr(hexchars, n);

    if (ptr) {
      size_t reg_num = ptr - hexchars;
      assert(reg_num >= 0 && reg_num <= 16);

      *out = reg_num & 0xF;
      
      LEX_MERGE_BRANCH(l, b);
      return true;
    }
  }

  return false;
}

[[noreturn]]
void report_error(Lex *l, const char *msg) {
  fprintf(stderr, "ERROR at line %s: %s\n", lex_cursor_pos_str(l->cursor), msg);
  exit(1);
}

void skip_comma(Lex *l) {
  if (!lex_skip(l, TERM, ","))
    report_error(l, "Missing comma");
}

// Opcodes which accepts Vx and a byte of data
// OP Vx, <data>
bool parse_Nxkk(Lex *l, char* keyword, uint8_t N, Opcode *out_op) {
  Lex b = LEX_BRANCH(l);
  
  if (lex_skip(&b, KEYWORD, keyword)) {
    uint8_t reg_x;
    if (parse_v_reg(&b, &reg_x)) {
      skip_comma(&b);

      uint16_t byte;
      if (parse_hex(&b, &byte)) {
        *out_op = (Opcode) { .N = N, .x = reg_x, .kk = byte & 0xFF };
        LEX_MERGE_BRANCH(l, b);
        return true;
      }
    }
  }

  return false;
}


// Opcode which accepts Vx and Vy
// OP Vx, Vy
bool parse_Nxyn(Lex *l, char* keyword, uint8_t N, uint8_t n, Opcode *out_op) {
  Lex b = LEX_BRANCH(l);

  if (lex_skip(&b, KEYWORD, keyword)) {
    uint8_t reg_x;
    if (parse_v_reg(&b, &reg_x)) {
      skip_comma(&b);

      uint8_t reg_y;
      if (parse_v_reg(&b, &reg_y)) {
        *out_op = (Opcode) { .N = N, .x = reg_x, .y = reg_y, .n = n & 0xF };
        LEX_MERGE_BRANCH(l, b);
        return true;
      }
    }
  }

  return false;
}

// Opcodes that only accepts Vx register
// OP Vx
bool parse_Nxnn(Lex *l, char *keyword, uint8_t N, uint8_t nn, Opcode *out_op) {
  Lex b = LEX_BRANCH(l);

  if (lex_skip(&b, KEYWORD, keyword)) {
    uint8_t reg_x;
    if (parse_v_reg(&b, &reg_x)) {
      *out_op = (Opcode) { .N = N, .x = reg_x, .kk = nn & 0xFF };
      
      LEX_MERGE_BRANCH(l, b);
      return true;
    }
  }

  return false;
}

// Opcodes which accepts 16bit addr or label as argument first argument
// OP <addr>
bool parse_Nnnn(Lex *l, const char *keyword, uint8_t N, Opcode *out_op, Label *out_label) {
  Lex b = LEX_BRANCH(l);

  if (lex_skip(&b, KEYWORD, keyword)) { 
    uint16_t addr;
    if (parse_hex(&b, &addr)) { 
      *out_op = (Opcode) { .N = N, .nnn = addr }; 
      
      LEX_MERGE_BRANCH(l, b);
      return true;
    }

    if (parse_label_ref(&b, out_label)) {
      *out_op = (Opcode) { .N = N }; 
      
      LEX_MERGE_BRANCH(l, b);
      return true;
    }
  }

  return false;
}

// Opcode that accpects I register and address
// OP I, <addr>
bool parse_NInnn(Lex *l, const char *keyword, uint8_t N, Opcode *out_op, Label *out_label) {
  Lex b = LEX_BRANCH(l);

  if (lex_skip(&b, KEYWORD, "LD")) {
    if (lex_skip(&b, ID, "I")) {
      skip_comma(&b);

      uint16_t addr;
      if (parse_hex(&b, &addr)) { 
        *out_op = (Opcode) { .N = N & 0xF, .nnn = addr }; 
        
        LEX_MERGE_BRANCH(l, b);
        return true;
      }

      if (parse_label_ref(&b, out_label)) {
        *out_op = (Opcode) { .N = N & 0xF }; 
        
        LEX_MERGE_BRANCH(l, b);
        return true;
      }
    }
  }

  return false;
}

// Opcode that accepst a target register (DT, ST, I) and a source Vx
// OP <T>, Vx
bool parse_NTxnn(Lex *l, const char *keyword, const char *target, uint8_t N, uint8_t nn, Opcode *out_op) {
  Lex b = LEX_BRANCH(l);

  if (lex_skip(&b, KEYWORD, keyword)) {
    if (lex_skip(&b, ID, target)) {
      skip_comma(&b);

      uint8_t reg_x;
      if (parse_v_reg(&b, &reg_x)) {
        *out_op = (Opcode) { .N = N & 0xF, .x = reg_x & 0xF, .kk = nn };

        LEX_MERGE_BRANCH(l, b);
        return true;
      }
    }
  }

  return false;
}

// Opcode that accepst a source register (DT, ST, I) and a target Vx
// OP Vx, <T>
bool parse_NSxnn(Lex *l, const char *keyword, const char *source, uint8_t N, uint8_t nn, Opcode *out_op) {
  Lex b = LEX_BRANCH(l);

  if (lex_skip(&b, KEYWORD, keyword)) {
    uint8_t reg_x;
    if (parse_v_reg(&b, &reg_x)) {
      skip_comma(&b);
    
      if (lex_skip(&b, ID, source)) {
        *out_op = (Opcode) { .N = N & 0xF, .x = reg_x & 0xF, .kk = nn };

        LEX_MERGE_BRANCH(l, b);
        return true;
      }
    }
  }

  return false;
}

bool parse_NNNN(Lex *l, char *keyword, Opcode op, Opcode *out_op) {
  if (lex_skip(l, KEYWORD, keyword)) {
    *out_op = op;
    return true;
  }

  return false;
}

bool parse_DRAW(Lex *l, Opcode *out_op) {
  if (lex_skip(l, KEYWORD, "DRW")) {
    uint8_t reg_x;
    if (parse_v_reg(l, &reg_x)) {
      skip_comma(l);

      uint8_t reg_y;
      if (parse_v_reg(l, &reg_y)) {
        skip_comma(l);
        
        uint16_t nibble;
        if (!parse_hex(l, &nibble)) report_error(l, "DRW command missing nibble");

        *out_op = (Opcode) { .N = 0xD, .x = reg_x & 0xF, .y = reg_y & 0xF, .n = nibble & 0xF };
        return true;
      }
    }

    report_error(l, "Invalid DRW syntax");
  }

  return false;
}

bool parse_opcode(Lex *l, Opcode *out_op, Label *out_label) {
  if (parse_NNNN(l, "CLS", OPCODE(00E0), out_op)) return true;    // 00E0
  if (parse_NNNN(l, "RET", OPCODE(00EE), out_op)) return true;    // 00EE
  if (parse_Nnnn(l, "JP",   1, out_op, out_label)) return true;   // 1nnn
  if (parse_Nnnn(l, "CALL", 2, out_op, out_label)) return true;   // 2nnn
  if (parse_Nxkk(l, "SE",   3,      out_op)) return true;         // 3xkk
  if (parse_Nxkk(l, "SNE",  4,      out_op)) return true;         // 4xkk
  if (parse_Nxyn(l, "SE",   5, 0,   out_op)) return true;         // 5xy0
  if (parse_Nxkk(l, "LD",   6,      out_op)) return true;         // 6xkk
  if (parse_Nxkk(l, "ADD",  7,      out_op)) return true;         // 7xkk
  if (parse_Nxyn(l, "LD",   8, 0,   out_op)) return true;         // 8xy0
  if (parse_Nxyn(l, "OR",   8, 1,   out_op)) return true;         // 8xy1
  if (parse_Nxyn(l, "AND",  8, 2,   out_op)) return true;         // 8xy2
  if (parse_Nxyn(l, "XOR",  8, 3,   out_op)) return true;         // 8xy3
  if (parse_Nxyn(l, "ADD",  8, 4,   out_op)) return true;         // 8xy4
  if (parse_Nxyn(l, "SUB",  8, 5,   out_op)) return true;         // 8xy5
  if (parse_Nxnn(l, "SHR",  8, 0x06,   out_op)) return true;      // 8x06 or 8xy6
  if (parse_Nxyn(l, "SUBN", 8, 7,   out_op)) return true;         // 8xy7
  if (parse_Nxnn(l, "SHL",  8, 0x0E, out_op)) return true;        // 8x0E or 8xyE
  if (parse_Nxyn(l, "SNE",  9, 0,   out_op)) return true;         // 9xy0
  if (parse_NInnn(l, "LD", 0xA, out_op, out_label)) return true;  // Annn
  if (parse_Nnnn(l, "JP",  0xB, out_op, out_label)) return true;  // Bnnn
  if (parse_Nxkk(l, "RND", 0xC,  out_op)) return true;            // Cxkk
  if (parse_DRAW(l, out_op)) return true;                         // Dxyn
  if (parse_Nxnn(l, "SKP",  0xE, 0x9E, out_op)) return true;      // Ex9E
  if (parse_Nxnn(l, "SKNP",  0xE, 0xA1, out_op)) return true;     // ExA1
  if (parse_NSxnn(l, "LD", "DT", 0xF, 0x07, out_op)) return true; // Fx07
  if (parse_NSxnn(l, "LD", "K", 0xF, 0x0A, out_op)) return true;  // Fx0A
  if (parse_NTxnn(l, "LD", "DT", 0xF, 0x15, out_op)) return true; // Fx15
  if (parse_NTxnn(l, "LD", "ST", 0xF, 0x18, out_op)) return true; // Fx18
  if (parse_NTxnn(l, "ADD", "I", 0xF, 0x1E, out_op)) return true; // Fx1E
  if (parse_NTxnn(l, "LD", "F", 0xF, 0x29, out_op)) return true;  // Fx29
  if (parse_NTxnn(l, "LD", "B", 0xF, 0x33, out_op)) return true;  // Fx33
  if (parse_NTxnn(l, "LD", "$I", 0xF, 0x55, out_op)) return true; // Fx55
  if (parse_NSxnn(l, "LD", "$I", 0xF, 0x65, out_op)) return true; // Fx65

  return false;
};

void parse_file(CompilerState *cs, const char *path) {
  const char* source = lex_read_file(path, NULL);
  Lex l = lex_init(LEX_TYPEARRAY(c8c_types), source);

  lex_print_hl(l, true);

  LexResult result;
  while (lex_current(&l, &result)) {
    Label label;
    if (parse_label_def(&l, &label)) {
      create_block(cs, label);
      continue;
    }

    Opcode op;
    Label ref_label = NULL;
    if (parse_opcode(&l, &op, &ref_label)) {
      push_op(cs, op);
      if (ref_label)
        push_ref(cs, ref_label);
      
      continue;
    }

    report_error(&l, "Invalid syntax");
  }

  if (result == LEX_INVALID_TOKEN)
    report_error(&l, "Invalid token");
}

int main() {
  CompilerState cs = create_compiler_state();
  parse_file(&cs, "./test.8");

  resolve_addresses(&cs);
  resolve_references(&cs);
  generate_binary(&cs);

  dump_compiler_state(cs);

  fwrite(cs.output.rom, 1, cs.output.rom_size, fopen("out.ch8", "w"));
}

/// LEX RULES

size_t c8c_rule_keyword(LexCursor cursor) {
  const char *keywords[] = {
    "CLS",  "DRW", "JP",   "ADD", "SUB",
    "SUBN", "LD",  "SHR",  "SHL", "SKP",
    "SKNP", "SE",  "SNE",  "OR",  "XOR",
    "AND",  "RND", "CALL", "RET"
  };

  for (size_t i = 0; i < ARRAYLEN(keywords); ++i) {
    size_t len = lex_match_keyword(cursor, keywords[i]);  
    if (len) return len;
  }

  return LEX_NO_MATCH;
}
  
size_t c8c_rule_hex(LexCursor cursor) {
  if (lex_cursor_ch(cursor) == '$') {
    size_t len = 0;
    for (;;) {
      lex_cursor_move(&cursor, 1);
    
      if (lex_match_chars(cursor, HEX_CHARS))
        len++;
      else
        break;
    };

    return len > 0? (len + 1) : LEX_NO_MATCH;
  }

  return LEX_NO_MATCH;
}

size_t c8c_rule_term(LexCursor cursor) { return lex_match_chars(cursor, ":,@?"); }
