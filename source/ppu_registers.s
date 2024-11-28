.EXPORT OAM_ADDRESS
.EXPORT OAM_DATA
.EXPORT PPU_ADDRESS
.EXPORT PPU_CONTROL
.EXPORT PPU_DATA
.EXPORT PPU_MASK
.EXPORT PPU_SCROLL
.EXPORT PPU_STATUS

.SEGMENT "PPU"
   PPU_CONTROL: ; miscellaneous settings (https://www.nesdev.org/wiki/PPU_registers#PPUCTRL)
   .RES 1
   PPU_MASK: ; rendering settings (https://www.nesdev.org/wiki/PPU_registers#PPUMASK)
   .RES 1
   PPU_STATUS: ; rendering events (https://www.nesdev.org/wiki/PPU_registers#PPUSTATUS)
   .RES 1
   OAM_ADDRESS: ; OAM address (https://www.nesdev.org/wiki/PPU_registers#OAMADDR)
   .RES 1
   OAM_DATA: ; OAM data (https://www.nesdev.org/wiki/PPU_registers#OAMDATA)
   .RES 1
   PPU_SCROLL: ; X and Y scroll (https://www.nesdev.org/wiki/PPU_registers#PPUSCROLL)
   .RES 1
   PPU_ADDRESS: ; VRAM address (https://www.nesdev.org/wiki/PPU_registers#PPUADDR)
   .RES 1
   PPU_DATA: ; VRAM data (https://www.nesdev.org/wiki/PPU_registers#PPUDATA)
   .RES 1