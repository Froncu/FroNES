.SEGMENT "HDR"
   .BYTE 'N', 'E', 'S', $1A ; constant ID (ASCII "NES" followed by MS-DOS end-of-file)
   .BYTE $02 ; size of PRG ROM in 16KB units
   .BYTE $01 ; size of CHR ROM in 8KB units (0 == CHR RAM)
   .BYTE $00 ; flags 6 (https://www.nesdev.org/wiki/INES#Flags_6)
   .BYTE $00 ; flags 7 (https://www.nesdev.org/wiki/INES#Flags_7)
   .BYTE $00 ; flags 8 (https://www.nesdev.org/wiki/INES#Flags_8)
   .BYTE $00 ; flags 9 (https://www.nesdev.org/wiki/INES#Flags_9)
   .BYTE $00 ; flags 10 (https://www.nesdev.org/wiki/INES#Flags_10)
   .BYTE $00, $00, $00, $00, $00 ; unused padding (some rippers put their names across bytes 7-15)