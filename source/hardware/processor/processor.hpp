#ifndef PROCESSOR_HPP
#define PROCESSOR_HPP

#include "hardware/memory/memory.hpp"
#include "instruction.hpp"
#include "pch.hpp"

namespace nes
{
   class Processor final
   {
      using BranchOperation = bool (Processor::*)() const;
      using ReadOperation = void (Processor::*)(Byte);
      using ModifyOperation = Byte (Processor::*)(Byte);
      using WriteOperation = Byte (Processor::*)();

      public:
         enum class Opcode : Byte
         {
            BRK_IMPLIED        = 0x00,
            ORA_X_INDIRECT     = 0x01,
            JAM_IMPLIED_02     = 0x02,
            SLO_X_INDIRECT     = 0x03,
            NOP_ZERO_PAGE_04   = 0x04,
            ORA_ZERO_PAGE      = 0x05,
            ASL_ZERO_PAGE      = 0x06,
            SLO_ZERO_PAGE      = 0x07,
            PHP_IMPLIED        = 0x08,
            ORA_IMMEDIATE      = 0x09,
            ASL_ACCUMULATOR    = 0x0A,
            ANC_IMMEDIATE_0B   = 0x0B,
            NOP_ABSOLUTE       = 0x0C,
            ORA_ABSOLUTE       = 0x0D,
            ASL_ABSOLUTE       = 0x0E,
            SLO_ABSOLUTE       = 0x0F,
            BPL_RELATIVE       = 0x10,
            ORA_INDIRECT_Y     = 0x11,
            JAM_IMPLIED_12     = 0x12,
            SLO_INDIRECT_Y     = 0x13,
            NOP_ZERO_PAGE_X_14 = 0x14,
            ORA_ZERO_PAGE_X    = 0x15,
            ASL_ZERO_PAGE_X    = 0x16,
            SLO_ZERO_PAGE_X    = 0x17,
            CLC_IMPLIED        = 0x18,
            ORA_ABSOLUTE_Y     = 0x19,
            NOP_IMPLIED_1A     = 0x1A,
            SLO_ABSOLUTE_Y     = 0x1B,
            NOP_ABSOLUTE_X_1C  = 0x1C,
            ORA_ABSOLUTE_X     = 0x1D,
            ASL_ABSOLUTE_X     = 0x1E,
            SLO_ABSOLUTE_X     = 0x1F,
            JSR_ABSOLUTE       = 0x20,
            AND_X_INDIRECT     = 0x21,
            JAM_IMPLIED_22     = 0x22,
            RLA_X_INDIRECT     = 0x23,
            BIT_ZERO_PAGE      = 0x24,
            AND_ZERO_PAGE      = 0x25,
            ROL_ZERO_PAGE      = 0x26,
            RLA_ZERO_PAGE      = 0x27,
            PLP_IMPLIED        = 0x28,
            AND_IMMEDIATE      = 0x29,
            ROL_ACCUMULATOR    = 0x2A,
            ANC_IMMEDIATE_2B   = 0x2B,
            BIT_ABSOLUTE       = 0x2C,
            AND_ABSOLUTE       = 0x2D,
            ROL_ABSOLUTE       = 0x2E,
            RLA_ABSOLUTE       = 0x2F,
            BMI_RELATIVE       = 0x30,
            AND_INDIRECT_Y     = 0x31,
            JAM_IMPLIED_32     = 0x32,
            RLA_INDIRECT_Y     = 0x33,
            NOP_ZERO_PAGE_X_34 = 0x34,
            AND_ZERO_PAGE_X    = 0x35,
            ROL_ZERO_PAGE_X    = 0x36,
            RLA_ZERO_PAGE_X    = 0x37,
            SEC_IMPLIED        = 0x38,
            AND_ABSOLUTE_Y     = 0x39,
            NOP_IMPLIED_3A     = 0x3A,
            RLA_ABSOLUTE_Y     = 0x3B,
            NOP_ABSOLUTE_X_3C  = 0x3C,
            AND_ABSOLUTE_X     = 0x3D,
            ROL_ABSOLUTE_X     = 0x3E,
            RLA_ABSOLUTE_X     = 0x3F,
            RTI_IMPLIED        = 0x40,
            EOR_X_INDIRECT     = 0x41,
            JAM_IMPLIED_42     = 0x42,
            SRE_X_INDIRECT     = 0x43,
            NOP_ZERO_PAGE_44   = 0x44,
            EOR_ZERO_PAGE      = 0x45,
            LSR_ZERO_PAGE      = 0x46,
            SRE_ZERO_PAGE      = 0x47,
            PHA_IMPLIED        = 0x48,
            EOR_IMMEDIATE      = 0x49,
            LSR_ACCUMULATOR    = 0x4A,
            ALR_IMMEDIATE_4B   = 0x4B,
            JMP_ABSOLUTE       = 0x4C,
            EOR_ABSOLUTE       = 0x4D,
            LSR_ABSOLUTE       = 0x4E,
            SRE_ABSOLUTE       = 0x4F,
            BVC_RELATIVE       = 0x50,
            EOR_INDIRECT_Y     = 0x51,
            JAM_IMPLIED_52     = 0x52,
            SRE_INDIRECT_Y     = 0x53,
            NOP_ZERO_PAGE_X_54 = 0x54,
            EOR_ZERO_PAGE_X    = 0x55,
            LSR_ZERO_PAGE_X    = 0x56,
            SRE_ZERO_PAGE_X    = 0x57,
            CLI_IMPLIED        = 0x58,
            EOR_ABSOLUTE_Y     = 0x59,
            NOP_IMPLIED_5A     = 0x5A,
            SRE_ABSOLUTE_Y     = 0x5B,
            NOP_ABSOLUTE_X_5C  = 0x5C,
            EOR_ABSOLUTE_X     = 0x5D,
            LSR_ABSOLUTE_X     = 0x5E,
            SRE_ABSOLUTE_X     = 0x5F,
            RTS_IMPLIED        = 0x60,
            ADC_X_INDIRECT     = 0x61,
            JAM_IMPLIED_62     = 0x62,
            RRA_X_INDIRECT     = 0x63,
            NOP_ZERO_PAGE_64   = 0x64,
            ADC_ZERO_PAGE      = 0x65,
            ROR_ZERO_PAGE      = 0x66,
            RRA_ZERO_PAGE      = 0x67,
            PLA_IMPLIED        = 0x68,
            ADC_IMMEDIATE      = 0x69,
            ROR_ACCUMULATOR    = 0x6A,
            ARR_IMMEDIATE      = 0x6B,
            JMP_INDIRECT       = 0x6C,
            ADC_ABSOLUTE       = 0x6D,
            ROR_ABSOLUTE       = 0x6E,
            RRA_ABSOLUTE       = 0x6F,
            BVS_RELATIVE       = 0x70,
            ADC_INDIRECT_Y     = 0x71,
            JAM_IMPLIED_72     = 0x72,
            RRA_INDIRECT_Y     = 0x73,
            NOP_ZERO_PAGE_X_74 = 0x74,
            ADC_ZERO_PAGE_X    = 0x75,
            ROR_ZERO_PAGE_X    = 0x76,
            RRA_ZERO_PAGE_X    = 0x77,
            SEI_IMPLIED        = 0x78,
            ADC_ABSOLUTE_Y     = 0x79,
            NOP_IMPLIED_7A     = 0x7A,
            RRA_ABSOLUTE_Y     = 0x7B,
            NOP_ABSOLUTE_X_7C  = 0x7C,
            ADC_ABSOLUTE_X     = 0x7D,
            ROR_ABSOLUTE_X     = 0x7E,
            RRA_ABSOLUTE_X     = 0x7F,
            NOP_IMMEDIATE_80   = 0x80,
            STA_X_INDIRECT     = 0x81,
            NOP_IMMEDIATE_82   = 0x82,
            SAX_X_INDIRECT     = 0x83,
            STY_ZERO_PAGE      = 0x84,
            STA_ZERO_PAGE      = 0x85,
            STX_ZERO_PAGE      = 0x86,
            SAX_ZERO_PAGE      = 0x87,
            DEY_IMPLIED        = 0x88,
            NOP_IMMEDIATE_89   = 0x89,
            TXA_IMPLIED        = 0x8A,
            ANE_IMMEDIATE      = 0x8B,
            STY_ABSOLUTE       = 0x8C,
            STA_ABSOLUTE       = 0x8D,
            STX_ABSOLUTE       = 0x8E,
            SAX_ABSOLUTE       = 0x8F,
            BCC_RELATIVE       = 0x90,
            STA_INDIRECT_Y     = 0x91,
            JAM_IMPLIED_92     = 0x92,
            SHA_INDIRECT_Y     = 0x93,
            STY_ZERO_PAGE_X    = 0x94,
            STA_ZERO_PAGE_X    = 0x95,
            STX_ZERO_PAGE_Y    = 0x96,
            SAX_ZERO_PAGE_Y    = 0x97,
            TYA_IMPLIED        = 0x98,
            STA_ABSOLUTE_Y     = 0x99,
            TXS_IMPLIED        = 0x9A,
            TAS_ABSOLUTE_Y     = 0x9B,
            SHY_ABSOLUTE_X     = 0x9C,
            STA_ABSOLUTE_X     = 0x9D,
            SHX_ABSOLUTE_Y     = 0x9E,
            SHA_ABSOLUTE_Y     = 0x9F,
            LDY_IMMEDIATE      = 0xA0,
            LDA_X_INDIRECT     = 0xA1,
            LDX_IMMEDIATE      = 0xA2,
            LAX_X_INDIRECT     = 0xA3,
            LDY_ZERO_PAGE      = 0xA4,
            LDA_ZERO_PAGE      = 0xA5,
            LDX_ZERO_PAGE      = 0xA6,
            LAX_ZERO_PAGE      = 0xA7,
            TAY_IMPLIED        = 0xA8,
            LDA_IMMEDIATE      = 0xA9,
            TAX_IMPLIED        = 0xAA,
            LXA_IMMEDIATE      = 0xAB,
            LDY_ABSOLUTE       = 0xAC,
            LDA_ABSOLUTE       = 0xAD,
            LDX_ABSOLUTE       = 0xAE,
            LAX_ABSOLUTE       = 0xAF,
            BCS_RELATIVE       = 0xB0,
            LDA_INDIRECT_Y     = 0xB1,
            JAM_IMPLIED_B2     = 0xB2,
            LAX_INDIRECT_Y     = 0xB3,
            LDY_ZERO_PAGE_X    = 0xB4,
            LDA_ZERO_PAGE_X    = 0xB5,
            LDX_ZERO_PAGE_Y    = 0xB6,
            LAX_ZERO_PAGE_Y    = 0xB7,
            CLV_IMPLIED        = 0xB8,
            LDA_ABSOLUTE_Y     = 0xB9,
            TSX_IMPLIED        = 0xBA,
            LAS_ABSOLUTE_Y     = 0xBB,
            LDY_ABSOLUTE_X     = 0xBC,
            LDA_ABSOLUTE_X     = 0xBD,
            LDX_ABSOLUTE_Y     = 0xBE,
            LAX_ABSOLUTE_Y     = 0xBF,
            CPY_IMMEDIATE      = 0xC0,
            CMP_X_INDIRECT     = 0xC1,
            NOP_IMMEDIATE_C2   = 0xC2,
            DCP_X_INDIRECT     = 0xC3,
            CPY_ZERO_PAGE      = 0xC4,
            CMP_ZERO_PAGE      = 0xC5,
            DEC_ZERO_PAGE      = 0xC6,
            DCP_ZERO_PAGE      = 0xC7,
            INY_IMPLIED        = 0xC8,
            CMP_IMMEDIATE      = 0xC9,
            DEX_IMPLIED        = 0xCA,
            SBX_IMMEDIATE      = 0xCB,
            CPY_ABSOLUTE       = 0xCC,
            CMP_ABSOLUTE       = 0xCD,
            DEC_ABSOLUTE       = 0xCE,
            DCP_ABSOLUTE       = 0xCF,
            BNE_RELATIVE       = 0xD0,
            CMP_INDIRECT_Y     = 0xD1,
            JAM_IMPLIED_D2     = 0xD2,
            DCP_INDIRECT_Y     = 0xD3,
            NOP_ZERO_PAGE_X_D4 = 0xD4,
            CMP_ZERO_PAGE_X    = 0xD5,
            DEC_ZERO_PAGE_X    = 0xD6,
            DCP_ZERO_PAGE_X    = 0xD7,
            CLD_IMPLIED        = 0xD8,
            CMP_ABSOLUTE_Y     = 0xD9,
            NOP_IMPLIED_DA     = 0xDA,
            DCP_ABSOLUTE_Y     = 0xDB,
            NOP_ABSOLUTE_X_DC  = 0xDC,
            CMP_ABSOLUTE_X     = 0xDD,
            DEC_ABSOLUTE_X     = 0xDE,
            DCP_ABSOLUTE_X     = 0xDF,
            CPX_IMMEDIATE      = 0xE0,
            SBC_X_INDIRECT     = 0xE1,
            NOP_IMMEDIATE_E2   = 0xE2,
            ISC_X_INDIRECT     = 0xE3,
            CPX_ZERO_PAGE      = 0xE4,
            SBC_ZERO_PAGE      = 0xE5,
            INC_ZERO_PAGE      = 0xE6,
            ISC_ZERO_PAGE      = 0xE7,
            INX_IMPLIED        = 0xE8,
            SBC_IMMEDIATE_E9   = 0xE9,
            NOP_IMPLIED_EA     = 0xEA,
            SBC_IMMEDIATE_EB   = 0xEB,
            CPX_ABSOLUTE       = 0xEC,
            SBC_ABSOLUTE       = 0xED,
            INC_ABSOLUTE       = 0xEE,
            ISC_ABSOLUTE       = 0xEF,
            BEQ_RELATIVE       = 0xF0,
            SBC_INDIRECT_Y     = 0xF1,
            JAM_IMPLIED_F2     = 0xF2,
            ISC_INDIRECT_Y     = 0xF3,
            NOP_ZERO_PAGE_X_F4 = 0xF4,
            SBC_ZERO_PAGE_X    = 0xF5,
            INC_ZERO_PAGE_X    = 0xF6,
            ISC_ZERO_PAGE_X    = 0xF7,
            SED_IMPLIED        = 0xF8,
            SBC_ABSOLUTE_Y     = 0xF9,
            NOP_IMPLIED_FA     = 0xFA,
            ISC_ABSOLUTE_Y     = 0xFB,
            NOP_ABSOLUTE_X_FC  = 0xFC,
            SBC_ABSOLUTE_X     = 0xFD,
            INC_ABSOLUTE_X     = 0xFE,
            ISC_ABSOLUTE_X     = 0xFF
         };

         enum class ProcessorStatusFlag : ProcessorStatus
         {
            C = 0b0000'0001,
            Z = 0b0000'0010,
            I = 0b0000'0100,
            D = 0b0000'1000,
            B = 0b0001'0000,
            _ = 0b0010'0000,
            V = 0b0100'0000,
            N = 0b1000'0000
         };

         static constexpr Word NMI_LOW{ 0xFF'FA };
         static constexpr Word NMI_HIGH{ NMI_LOW + 1 };
         static constexpr Word RESET_LOW{ 0xFF'FC };
         static constexpr Word RESET_HIGH{ RESET_LOW + 1 };
         static constexpr Word IRQ_LOW{ 0xFF'FE };
         static constexpr Word IRQ_HIGH{ IRQ_LOW + 1 };

         Processor(Memory& memory);
         Processor(Processor const&) = delete;
         Processor(Processor&&) = delete;

         ~Processor() = default;

         auto operator=(Processor const&) -> Processor& = delete;
         auto operator=(Processor&&) -> Processor& = delete;

         auto tick() -> bool;
         auto reset() -> void;

         [[nodiscard]] auto cycle() const -> Cycle;
         [[nodiscard]] auto accumulator() const -> Accumulator;
         [[nodiscard]] auto x() const -> Index;
         [[nodiscard]] auto y() const -> Index;
         [[nodiscard]] auto stack_pointer() const -> StackPointer;
         [[nodiscard]] auto processor_status() const -> ProcessorStatus;

         ProgramCounter program_counter{};

      private:
         // Addressing modes
         [[nodiscard]] auto relative(BranchOperation operation) -> Instruction;

         [[nodiscard]] auto immediate(ReadOperation operation) -> Instruction;
         [[nodiscard]] auto absolute(ReadOperation operation) -> Instruction;
         [[nodiscard]] auto zero_page(ReadOperation operation) -> Instruction;
         [[nodiscard]] auto zero_page_indexed(ReadOperation operation, Index index) -> Instruction;
         [[nodiscard]] auto absolute_indexed(ReadOperation operation, Index index) -> Instruction;
         [[nodiscard]] auto x_indirect(ReadOperation operation) -> Instruction;
         [[nodiscard]] auto indirect_y(ReadOperation operation) -> Instruction;

         [[nodiscard]] auto accumulator(ModifyOperation operation) -> Instruction;
         [[nodiscard]] auto absolute(ModifyOperation operation) -> Instruction;
         [[nodiscard]] auto zero_page(ModifyOperation operation) -> Instruction;
         [[nodiscard]] auto zero_page_indexed(ModifyOperation operation, Index index) -> Instruction;
         [[nodiscard]] auto absolute_indexed(ModifyOperation operation, Index index) -> Instruction;
         [[nodiscard]] auto x_indirect(ModifyOperation operation) -> Instruction;
         [[nodiscard]] auto indirect_y(ModifyOperation operation) -> Instruction;

         [[nodiscard]] auto absolute(WriteOperation operation) -> Instruction;
         [[nodiscard]] auto zero_page(WriteOperation operation) -> Instruction;
         [[nodiscard]] auto zero_page_indexed(WriteOperation operation, Index index) -> Instruction;
         [[nodiscard]] auto absolute_indexed(WriteOperation operation, Index index) -> Instruction;
         [[nodiscard]] auto x_indirect(WriteOperation operation) -> Instruction;
         [[nodiscard]] auto indirect_y(WriteOperation operation) -> Instruction;
         // ---

         // Implied instructions
         [[nodiscard]] auto RST() -> Instruction;
         [[nodiscard]] auto BRK() -> Instruction;
         [[nodiscard]] auto PHP() -> Instruction;
         [[nodiscard]] auto CLC() -> Instruction;
         [[nodiscard]] auto JSR() -> Instruction;
         [[nodiscard]] auto PLP() -> Instruction;
         [[nodiscard]] auto SEC() -> Instruction;
         [[nodiscard]] auto RTI() -> Instruction;
         [[nodiscard]] auto PHA() -> Instruction;
         [[nodiscard]] auto JMP_absolute() -> Instruction;
         [[nodiscard]] auto CLI() -> Instruction;
         [[nodiscard]] auto RTS() -> Instruction;
         [[nodiscard]] auto PLA() -> Instruction;
         [[nodiscard]] auto JMP_indirect() -> Instruction;
         [[nodiscard]] auto SEI() -> Instruction;
         [[nodiscard]] auto DEY() -> Instruction;
         [[nodiscard]] auto TXA() -> Instruction;
         [[nodiscard]] auto TYA() -> Instruction;
         [[nodiscard]] auto TXS() -> Instruction;
         [[nodiscard]] auto TAY() -> Instruction;
         [[nodiscard]] auto TAX() -> Instruction;
         [[nodiscard]] auto CLV() -> Instruction;
         [[nodiscard]] auto TSX() -> Instruction;
         [[nodiscard]] auto INY() -> Instruction;
         [[nodiscard]] auto DEX() -> Instruction;
         [[nodiscard]] auto CLD() -> Instruction;
         [[nodiscard]] auto INX() -> Instruction;
         [[nodiscard]] auto NOP() -> Instruction;
         [[nodiscard]] auto SED() -> Instruction;
         // ---

         // Branch operations
         [[nodiscard]] auto BPL() const -> bool;
         [[nodiscard]] auto BMI() const -> bool;
         [[nodiscard]] auto BVC() const -> bool;
         [[nodiscard]] auto BVS() const -> bool;
         [[nodiscard]] auto BCC() const -> bool;
         [[nodiscard]] auto BCS() const -> bool;
         [[nodiscard]] auto BNE() const -> bool;
         [[nodiscard]] auto BEQ() const -> bool;
         // ---

         // Read operations
         auto ORA(Byte value) -> void;
         auto LDA(Byte value) -> void;
         auto AND(Byte value) -> void;
         auto BIT(Byte value) -> void;
         auto EOR(Byte value) -> void;
         auto ADC(Byte value) -> void;
         auto LDY(Byte value) -> void;
         auto LDX(Byte value) -> void;
         auto CPY(Byte value) -> void;
         auto CMP(Byte value) -> void;
         auto CPX(Byte value) -> void;
         auto SBC(Byte value) -> void;
         // ---

         // Modify operations
         [[nodiscard]] auto ASL(Byte value) -> Byte;
         [[nodiscard]] auto ROL(Byte value) -> Byte;
         [[nodiscard]] auto LSR(Byte value) -> Byte;
         [[nodiscard]] auto ROR(Byte value) -> Byte;
         [[nodiscard]] auto DEC(Byte value) -> Byte;
         [[nodiscard]] auto INC(Byte value) -> Byte;
         // ---

         // Write operations
         auto STA() -> Byte;
         auto STY() -> Byte;
         auto STX() -> Byte;
         // ---

         // Helper functions
         [[nodiscard]] auto instruction_from_opcode(Opcode opcode) -> Instruction;

         auto change_processor_status_flag(ProcessorStatusFlag flag, bool set) -> void;
         [[nodiscard]] auto processor_status_flag(ProcessorStatusFlag flag) const -> bool;
         auto update_zero_and_negative_flag(Byte value) -> void;

         auto write_to_stack(Byte value) const -> void;
         [[nodiscard]] auto read_from_stack() const -> Byte;

         [[nodiscard]] static constexpr auto low_byte(Word source) -> Byte;
         [[nodiscard]] static constexpr auto high_byte(Word source) -> Byte;
         [[nodiscard]] static constexpr auto assemble_word(Byte high, Byte low) -> Word;
         [[nodiscard]] static constexpr auto assign_low_byte(Word target, Byte value) -> Word;
         [[nodiscard]] static constexpr auto assign_high_byte(Word target, Byte value) -> Word;
         [[nodiscard]] static constexpr auto add_with_overflow(Byte left, Byte right) -> std::pair<Byte, bool>;
         [[nodiscard]] static constexpr auto add_with_overflow(Byte left, SignedByte right) -> std::pair<Byte, SignedByte>;
         // ---

         Memory& memory_;

         Cycle cycle_{};
         Accumulator accumulator_{};
         Index x_{};
         Index y_{};
         StackPointer stack_pointer_{ 0xFF };
         ProcessorStatus processor_status_{};

         Opcode current_opcode_{};
         std::optional<Instruction> current_instruction_{ RST() };
   };
}

#endif