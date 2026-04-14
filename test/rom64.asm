main:
  JP @loop
  LD v1, $D       ; Y 

  ; Traslation
  LD v3, $0       ; X'
  LD v4, $0       ; Y'
loop:
  CLS

  LD v0, $1b      ; X
  LD v2, $6       ; SPR
  CALL @drw_digit

  LD v0, $21      ; X
  LD v2, $4       ; SPR
  CALL @drw_digit

  ADD v3, $1
  ADD v4, $1

  CALL @sync
  JP @loop

sync:
  CALL @save

  ; Set timer 
  LD v0, $1
  LD DT, v0

  sync_loop:
    ; Check if timer is zero
    LD v0, DT
    SNE v0, $0
      JP @sync_break
    
    JP @sync_loop

  sync_break:
    CALL @restore
    RET

drw_digit:
  CALL @save

  ; Add translation
  ADD v0, v3
  ADD v1, v4

  LD  F, v2
  DRW v0, v1, $5

  CALL @restore
  RET

save:
  LD I, $100
  LD $I, vF
  RET

restore:
  LD I, $100
  LD vF, $I
  RET
