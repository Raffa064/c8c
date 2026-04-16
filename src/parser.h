#ifndef C8C_PARSER_H
#define C8C_PARSER_H

#include "../libs/lex.h"
#include "core.h"

[[noreturn]]
void report_error(Lex *l, const char *msg);

void skip_comma(Lex *l);

// label:?
bool parse_label_def(Lex *l, Label *out, bool *is_raw);

// @label
bool parse_label_ref(Lex *l, Label *out);

// $200
bool parse_hex(Lex *l, uint16_t *out);

// v0
bool parse_v_reg(Lex *l, uint8_t *out);

// Opcodes which accepts Vx and a byte of data
// OP Vx, <data>
bool parse_Nxkk(Lex *l, char* keyword, uint8_t N, Opcode *out_op);

// Opcode which accepts Vx and Vy
// OP Vx, Vy
bool parse_Nxyn(Lex *l, char* keyword, uint8_t N, uint8_t n, Opcode *out_op);

// Opcodes that only accepts Vx register
// OP Vx
bool parse_Nxnn(Lex *l, char *keyword, uint8_t N, uint8_t nn, Opcode *out_op);

// Opcodes which accepts 16bit addr or label as argument first argument
// OP <addr>
bool parse_Nnnn(Lex *l, const char *keyword, uint8_t N, Opcode *out_op, Label *out_label);

// Opcode that accpects I register and address
// OP I, <addr>
bool parse_NInnn(Lex *l, const char *keyword, uint8_t N, Opcode *out_op, Label *out_label);

// Opcode that accepst a target register (DT, ST, I) and a source Vx
// OP <T>, Vx
bool parse_NTxnn(Lex *l, const char *keyword, const char *target, uint8_t N, uint8_t nn, Opcode *out_op);

// Opcode that accepst a source register (DT, ST, I) and a target Vx
// OP Vx, <T>
bool parse_NSxnn(Lex *l, const char *keyword, const char *source, uint8_t N, uint8_t nn, Opcode *out_op);

// Opcodes with no arguments
bool parse_NNNN(Lex *l, char *keyword, Opcode op, Opcode *out_op);

bool parse_DRAW(Lex *l, Opcode *out_op);

bool parse_opcode(Lex *l, Opcode *out_op, Label *out_label);

void parse_file(CompilerState *cs, const char *path);

#endif
