#include "macros.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LEX_IMPLEMENTATION

#include "lexer.h"
#include "core.h"
#include "parser.h"

typedef struct {
  const char *output_path;

  bool print_compiler_dump;
  bool print_highlight;

  CompilerInput compiler_input;
} C8CParams;

void print_usage(FILE *out) {
  fprintf(
    out, 
    "c8c [options] <file1.asm, file2.asm,....>\n"
    "OPTIONS\n"
    "  -o <output.ch8>  Set output file path (df: out.ch8)\n"
    "  -d               Print compiler dump\n"
    "  -H               Print input file with highlighted syntax\n"
    "  -E <label>       Set entrypoint label (df: main)\n"
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
    .output_path = "out.ch8",
    .compiler_input.opt.entrypoint = "main"
  };

  // c8c [opt] input.asm
  for (int i = 1; i < argc; i++) {
    char *opt = argv[i];

    if (opt[0] == '-') {
      if (strcmp(opt, "-o") == 0) {
        params.output_path = argv[++i];
      } else if (strcmp(opt, "-h") == 0) {
        print_usage(stdout);
        exit(0);
      } else if (strcmp(opt, "-d") == 0) {
        params.print_compiler_dump = true;
      } else if (strcmp(opt, "-H") == 0) {
        print_highlight(argv[++i]);
      } else if (strcmp(opt, "-E") == 0) {
        params.compiler_input.opt.main_jump = true;
        params.compiler_input.opt.entrypoint = argv[++i];
      } else {
        print_usage(stderr);
        fprintf(stderr, "Invalid option: '%s'\n", opt);
        exit(1);
      }
    } else {
      da_append(params.compiler_input.input_files, opt); // append file
    }
  }

  CompilerState cs = create_compiler_state(params.compiler_input);
  
  parse_input_files(&cs);
  resolve_addresses(&cs);
  resolve_references(&cs);
  generate_binary(&cs);

  if (params.print_compiler_dump)
    dump_compiler_state(cs);

  write_to_file(cs, params.output_path);
}
