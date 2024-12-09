.IMPORT DMC_FREQUENCY
.IMPORT JOYPAD_2
.IMPORT MAIN
.IMPORT NAMETABLE_INITIAL
.IMPORT OAM_INITIAL
.IMPORT OAM_LOCAL
.IMPORT PALETTE
.IMPORT PPU_ADDRESS
.IMPORT PPU_CONTROL
.IMPORT PPU_DATA
.IMPORT PPU_MASK
.IMPORT PPU_SCROLL
.IMPORT PPU_STATUS
.IMPORT TRANSFER_OAM
.INCLUDE "constants.inc"

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
   
   ; intialise the local OAM (the X register contains zero after the overflow in the CLEAR_RAM loop)
   INITIALISE_LOCAL_OAM:
   LDA OAM_INITIAL,X
   STA OAM_LOCAL,X
   INX
   BNE INITIALISE_LOCAL_OAM

   ; wait for the first 2 VBlanks to let the PPU stabilise before writing to it
   LDY #2
   JSR WAIT_VBLANK_Y_TIMES

   ; transfer the local OAM data into the PPU
   JSR TRANSFER_OAM

   ; transfer the initial nametable to the PPU
   LDA PPU_STATUS ; clear the PPU's internal W register used for tracking whether the following write is the first or the second one (high or low byte)

   ; set the high byte of the target address in the PPU
   LDA #$20
   STA PPU_ADDRESS

   STX PPU_ADDRESS ; set the low byte of the target address in the PPU (the X register contains zero after the overflow in the INITIALISE_LOCAL_OAM loop)
   NAMETABLE_INITIAL_QUARTER_1 = NAMETABLE_INITIAL
   NAMETABLE_INITIAL_QUARTER_2 = NAMETABLE_INITIAL_QUARTER_1 + QUARTER_NAMETABLE_SIZE
   NAMETABLE_INITIAL_QUARTER_3 = NAMETABLE_INITIAL_QUARTER_2 + QUARTER_NAMETABLE_SIZE
   NAMETABLE_INITIAL_QUARTER_4 = NAMETABLE_INITIAL_QUARTER_3 + QUARTER_NAMETABLE_SIZE
   TRANSFER_NAMETABLE_QUARTER_1:
   LDA NAMETABLE_INITIAL_QUARTER_1,X
   STA PPU_DATA
   INX
   BNE TRANSFER_NAMETABLE_QUARTER_1
   TRANSFER_NAMETABLE_QUARTER_2:
   LDA NAMETABLE_INITIAL_QUARTER_2,X
   STA PPU_DATA
   INX
   BNE TRANSFER_NAMETABLE_QUARTER_2
   TRANSFER_NAMETABLE_QUARTER_3:
   LDA NAMETABLE_INITIAL_QUARTER_3,X
   STA PPU_DATA
   INX
   BNE TRANSFER_NAMETABLE_QUARTER_3
   TRANSFER_NAMETABLE_QUARTER_4:
   LDA NAMETABLE_INITIAL_QUARTER_4,X
   STA PPU_DATA
   INX
   BNE TRANSFER_NAMETABLE_QUARTER_4

   ; transfer the palette to the PPU
   LDA PPU_STATUS ; clear the PPU's internal W register used for tracking whether the following write is the first or the second one (high or low byte)

   ; set the high byte of the target address in the PPU
   LDA #$3F
   STA PPU_ADDRESS

   STX PPU_ADDRESS ; set the low byte of the target address in the PPU (the X register contains zero after the overflow in the TRANSFER_NAMETABLE_QUARTER_4+ loop)
   TRANSFER_PALETTE:
   LDA PALETTE,X
   STA PPU_DATA
   INX
   CPX #32
   BNE TRANSFER_PALETTE

   ; reset the scroll (writes to PPU_ADDRESS can overwrite the scroll position, so the reset must happen here)
   LDA PPU_STATUS ; clear the PPU's internal W register used for tracking whether the following write is the first or the second one (high or low byte)
   LDA #0
   STA PPU_SCROLL
   STA PPU_SCROLL
   STA PPU_CONTROL

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