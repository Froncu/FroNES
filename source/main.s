.EXPORT MAIN

.SEGMENT "CDE"
   .PROC MAIN
   LDA #$FF
   JMP MAIN
   .ENDPROC