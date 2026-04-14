main:
  LD v0, $B // A
  LD v1, $A // B

  CALL @multiply
  CALL @load_bcd

  LD v2, v3
  CALL @draw_v2 
  
  LD v2, v4
  CALL @draw_v2

  LD v2, v5
  CALL @draw_v2 

end: JP @end

load_bcd:
  ; Take value at v0, and store it's bcd representation at v3, v4, and v5 registers
  LD I, $100 ; Move I to memory address $100
  LD B, v0   ; Load BCD representation of v0 into memory 
  LD I, $FD  ; Move 3 bytes backward
  LD v6, $I  ; Copy memory to registers
  RET

draw_v2:
  ; Draw v2 value on screen at position (v0, v1)
  LD F, v2
  DRW v0, v1, $5
  ADD v0, $5
  RET

multiply:
  ; Takes v0 and v1, multiply them, store result on v0
  LD v2, v0
  mul_lp:
    ADD v1, $FF ; Subtract 1 from v1
    
    SNE v1, $0
      JP @mul_brk
    
    ADD v0, v2
    JP @mul_lp

  mul_brk:
  RET
