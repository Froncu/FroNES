.EXPORT PALETTE

.SEGMENT "CHR"
   .INCBIN "resources/background.chr"
   .INCBIN "resources/sprites.chr"

.SEGMENT "RES"
   PALETTE:
   .INCBIN "resources/palettes.pal"