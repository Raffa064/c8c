#define LEX_IMPLEMENTATION

#include "core.h"
#include "parser.h"

int main() {
  CompilerState cs = create_compiler_state();
  parse_file(&cs, "./test.8");

  resolve_addresses(&cs);
  resolve_references(&cs);
  generate_binary(&cs);

  dump_compiler_state(cs);

  fwrite(cs.output.rom, 1, cs.output.rom_size, fopen("out.ch8", "w"));
}
