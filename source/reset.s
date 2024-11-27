.IMPORT PPU_CONTROL
.IMPORT PPU_MASK
.IMPORT DMC_FREQUENCY
.IMPORT JOYPAD_2
.IMPORT PPU_STATUS
.IMPORT OAM
.IMPORT MAIN

.EXPORT RESET

.SEGMENT "CDE"
   .PROC RESET
   SEI ; mask interrupts

   LDA #$00
   STA PPU_CONTROL ; disable VBlank NMIs
   STA PPU_MASK ; disable rendering
   STA DMC_FREQUENCY ; disable DMC IRQs

   ; disable APU frame IRQs
   LDA #$40
   STA JOYPAD_2

   CLD ; clear decimal flag

   ; initialise the stack
   LDX #$FF
   TXS

   ; wait for the first VBlank
   WAIT_VBLANK_1:
   BIT PPU_STATUS
   BPL WAIT_VBLANK_1

   ; clear the RAM
   LDA #$00
   LDX #$00
   CLEAR_RAM:
   STA $0000,X
   STA $0100,X
   STA $0200,X
   STA $0300,X
   STA $0400,X
   STA $0500,X
   STA $0600,X
   STA $0700,X
   INX
   BNE CLEAR_RAM

   ; set all the sprites' Y position to 0 in order to render them below the visible screen area
   LDA #$FF
   LDX #$00
   CLEAR_OAM:
   STA OAM,X
   INX
   INX
   INX
   INX
   BNE CLEAR_OAM

   ; wait for the second VBlank
   WAIT_VBLANK_2:
   BIT PPU_STATUS
   BPL WAIT_VBLANK_2

   ; enable VBlank NMIs
   LDA #%10000000
   STA PPU_CONTROL

   JMP MAIN
   .ENDPROC