#ifndef C8C_MACROS_H
#define C8C_MACROS_H

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

#endif
