.IMPORT NMI_READY
.IMPORT READ_CONTROLLERS

.EXPORT MAIN

.SEGMENT "CDE"
   .PROC MAIN
   LDA NMI_READY
   BNE MAIN

   JSR READ_CONTROLLERS

   LDA #1
   STA NMI_READY
   JMP MAIN
   .ENDPROC