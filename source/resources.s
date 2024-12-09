.EXPORT NAMETABLE_INITIAL
.EXPORT OAM_INITIAL
.EXPORT PALETTE

.SEGMENT "CHR"
   .INCBIN "resources/background.chr"
   .INCBIN "resources/sprites.chr"

.SEGMENT "RES"
   PALETTE:
   .INCBIN "resources/palettes.pal"

   NAMETABLE_INITIAL:
   .INCBIN "resources/nametable_initial.nam"

   OAM_INITIAL:
   .INCBIN "resources/oam_initial.oam"