#include "macros.h"
#include "lexer.h"

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
