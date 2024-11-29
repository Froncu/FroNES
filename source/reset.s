.IMPORT DMC_FREQUENCY
.IMPORT JOYPAD_2
.IMPORT MAIN
.IMPORT OAM_LOCAL
.IMPORT PALETTE
.IMPORT PPU_ADDRESS
.IMPORT PPU_CONTROL
.IMPORT PPU_DATA
.IMPORT PPU_MASK
.IMPORT PPU_STATUS

.EXPORT RESET

.SEGMENT "CDE"
   .PROC RESET
   SEI ; disable IRQs
   LDA #$00
   STA PPU_CONTROL ; set all the PPU miscellaneous settings to 0
   STA PPU_MASK ; disable all the PPU rendering settings
   STA DMC_FREQUENCY ; disable DMC IRQs

   ; disable APU frame IRQs
   LDA #$40
   STA JOYPAD_2

   CLD ; clear decimal flag

   ; initialise the stack
   LDX #$FF
   TXS

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

   ; set all the sprites' Y position to 0 in order to render them below the visible screen area when VBlank occurs (the X register contains zero after the overflow in the CLEAR_RAM loop)
   LDA #$FF
   CLEAR_OAM_LOCAL:
   STA OAM_LOCAL,X
   INX
   INX
   INX
   INX
   BNE CLEAR_OAM_LOCAL

   ; wait for the first 2 VBlanks to let the PPU stabilise before writing the palettes into it's VRAM
   LDY #2
   JSR WAIT_VBLANK_Y_TIMES
   
   ; transfer the palette to the PPU
   LDA PPU_STATUS ; clear the PPU's internal W register used for tracking whether the following write is the first or the second one (high or low byte)

   ; set the high byte of the target address in the PPU
   LDA #$3F
   STA PPU_ADDRESS

   STX PPU_ADDRESS ; set the low byte of the target address in the PPU (the X register contains zero after the overflow in the CLEAR_OAM loop)
   TRANSFER_PALETTE:
   LDA PALETTE,X
   STA PPU_DATA
   INX
   CPX #32
   BNE TRANSFER_PALETTE

   ; wait for one more VBlank before enabling renderering and NMIs just to be sure
   LDY #1
   JSR WAIT_VBLANK_Y_TIMES

   ; show both the background and the sprites in the leftmost 8 pixels of the screen and enable rendering for both the background and the sprites
   LDA #%00011110
   STA PPU_MASK
   
   ; enable VBlank NMIs and assign the second pattern table to the sprites
   LDA #%10001000
   STA PPU_CONTROL

   JMP MAIN
   
   ; wait for VBlank the amount of times specified by the value stored in the Y register
   WAIT_VBLANK_Y_TIMES:
   BIT PPU_STATUS
   BPL WAIT_VBLANK_Y_TIMES
   DEY
   BNE WAIT_VBLANK_Y_TIMES
   RTS
   .ENDPROC