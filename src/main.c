#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEX_IMPLEMENTATION

#include "lexer.h"
#include "core.h"
#include "parser.h"

typedef struct {
  struct {
    const char *input;
    const char *output;
  } path;

  bool print_compiler_dump;
  bool print_highlight;
} C8CParams;

void print_usage(FILE *out) {
  fprintf(
    out, 
    "c8c [options] <input.asm>\n"
    "OPTIONS\n"
    "  -o <output.ch8>  Set output file path (df: out.ch8)\n"
    "  -d               Print compiler dump\n"
    "  -H               Print input file with highlighted syntax\n"
    "  -h               Show this message)\n"
  );
}

[[noreturn]]
void print_highlight(const char *path) {
  Lex l = create_lexer(path);
  lex_print_hl(l, true);
  exit(0);
}

int main(int argc, char **argv) {
  C8CParams params = {
    .path.output = "out.ch8",
  };

  // c8c [opt] input.asm
  for (int i = 1; i < argc; i++) {
    char *opt = argv[i];

    if (opt[0] == '-') {
      if (strcmp(opt, "-o") == 0) {
        params.path.output = argv[++i];
      } else if (strcmp(opt, "-h") == 0) {
        print_usage(stdout);
        exit(0);
      } else if (strcmp(opt, "-d") == 0) {
        params.print_compiler_dump = true;
      } else if (strcmp(opt, "-H") == 0) {
        params.print_highlight = true;
      } else {
        print_usage(stderr);
        fprintf(stderr, "Invalid option: '%s'\n", opt);
        exit(1);
      }
    } else {
      params.path.input = opt;
    }
  }

  if (params.print_highlight)
    print_highlight(params.path.input);


  CompilerState cs = create_compiler_state();
  parse_file(&cs, params.path.input);

  resolve_addresses(&cs);
  resolve_references(&cs);
  generate_binary(&cs);

  if (params.print_compiler_dump)
    dump_compiler_state(cs);

  write_to_file(cs, params.path.output);
}
