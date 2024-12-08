.IMPORT NMI_READY
.IMPORT OAM_ADDRESS
.IMPORT OAM_DMA
.IMPORT OAM_LOCAL
.IMPORT PPU_MASK

.EXPORT NMI
.EXPORT TRANSFER_OAM

.SEGMENT "CDE"
   .PROC NMI
   ; push the registers on the stack
   PHA
   TXA
   PHA
   TYA
   PHA
   PHP

   ; branch to the end of the procedure when NMI_READY == 0
   LDA NMI_READY
   BEQ END

   ; transfer the local OAM data into the PPU
   JSR TRANSFER_OAM

   ; flag the PPU update complete
   LDX #0
   STX NMI_READY

   ; retrieve the registers from the stack
   END:
   PLP
   PLA
   TAY
   PLA
   TAX
   PLA
   RTI
   .ENDPROC

   .PROC TRANSFER_OAM
   ; set the starting address to write to in the PPUs OAM
   LDA #$00
   STA OAM_ADDRESS
   
   LDA #>OAM_LOCAL ; load the high byte of the local OAM's address
   STA OAM_DMA ; set the high byte of the local OAM's address in the OAM_DMA, effectively triggering the DMA

   RTS
   .ENDPROC