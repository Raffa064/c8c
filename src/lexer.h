#ifndef C8C_LEXER_H
#define C8C_LEXER_H

#define LEX_USE_XMACRO
#include "../libs/lex.h"

#define C8C(X)\
  X(WS,      lex_builtin_rule_ws,              .skip = true)\
  X(COMMENT, lex_builtin_rule_asmlike_comment, .skip = true)\
  X(KEYWORD, c8c_rule_keyword)\
  X(TERM,    c8c_rule_term)\
  X(HEX,     c8c_rule_hex)\
  X(ID,      lex_builtin_rule_id)

LEX_XMACRO_FRAMEWORK(C8C, C8CTypes, c8c_types);

Lex create_lexer(const char *path);

#endif
