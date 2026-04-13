#include "exceptions/unsupported_opcode.hpp"
#include "processor.hpp"

namespace nes
{
   Processor::Processor(Memory& memory)
      : memory_{memory}
   {
   }

   auto Processor::tick() -> bool
   {
      ++cycle_;

      if (not current_instruction_)
      {
         current_opcode_ = static_cast<Opcode>(memory_.read(program_counter));
         ++program_counter;
         current_instruction_ = instruction_from_opcode(current_opcode_);
      }
      else if (current_instruction_->tick())
      {
         std::optional prefetched_instruction{current_instruction_->prefetched_instruction()};
         current_instruction_ = std::move(prefetched_instruction);
         return true;
      }

      return false;
   }

   auto Processor::reset() -> void
   {
      cycle_ = 0;
      current_opcode_ = {};
      current_instruction_ = RST();
   }

   auto Processor::cycle() const -> Cycle
   {
      return cycle_;
   }

   auto Processor::accumulator() const -> Accumulator
   {
      return accumulator_;
   }

   auto Processor::x() const -> Index
   {
      return x_;
   }

   auto Processor::y() const -> Index
   {
      return y_;
   }

   auto Processor::stack_pointer() const -> StackPointer
   {
      return stack_pointer_;
   }

   auto Processor::processor_status() const -> ProcessorStatus
   {
      return processor_status_;
   }

   auto Processor::relative(BranchOperation const operation) -> Instruction
   {
      // fetch operand, increment PC
      auto const operand{static_cast<SignedByte>(memory_.read(program_counter))};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch opcode of next instruction, if branch is taken, add operand to PCL, otherwise increment PC
      Byte next_opcode{memory_.read(program_counter)};
      if (std::invoke(operation, this))
      {
         auto const [program_counter_low, overflow]{add_with_overflow(low_byte(program_counter), operand)};
         program_counter = assign_low_byte(program_counter, program_counter_low);
         co_await std::suspend_always{};

         // fetch opcode of next instruction, fix PCH, if it did not change, increment PC (+)
         next_opcode = memory_.read(program_counter);
         if (overflow)
         {
            auto const program_counter_high{static_cast<Byte>(high_byte(program_counter) + overflow)};
            program_counter = assign_high_byte(program_counter, program_counter_high);
            co_await std::suspend_always{};

            // fetch opcode of next instruction, increment PC (!)
            next_opcode = memory_.read(program_counter);
         }
      }

      ++program_counter;
      co_return instruction_from_opcode(static_cast<Opcode>(next_opcode));
   }

   auto Processor::immediate(ReadOperation const operation) -> Instruction
   {
      // fetch value, increment PC
      Byte const value{memory_.read(program_counter)};
      ++program_counter;

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   auto Processor::absolute(ReadOperation const operation) -> Instruction
   {
      // fetch low byte of address, increment PC
      Byte const low_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Byte const high_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address
      Word const effective_address{assemble_word(high_byte_of_address, low_byte_of_address)};
      Byte const value{memory_.read(effective_address)};

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   auto Processor::zero_page(ReadOperation const operation) -> Instruction
   {
      // fetch address, increment PC
      Word const address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address
      Byte const value{memory_.read(address)};

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   auto Processor::zero_page_indexed(ReadOperation const operation, Index const index) -> Instruction
   {
      // fetch address, increment PC
      Byte address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from address, add index register to it
      std::ignore = memory_.read(address); // ???
      address += index;
      co_await std::suspend_always{};

      // read from effective address
      Byte const value{memory_.read(address)};

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   auto Processor::absolute_indexed(ReadOperation const operation, Index const index) -> Instruction
   {
      // fetch low byte of address, increment PC
      Byte const low_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Byte high_byte_of_address{memory_.read(program_counter)};
      auto const [low_byte, overflow]{add_with_overflow(low_byte_of_address, index)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      Word effective_address{assemble_word(high_byte_of_address, low_byte)};
      Byte value{memory_.read(effective_address)};
      if (overflow)
      {
         ++high_byte_of_address;
         co_await std::suspend_always{};

         // re-read from effective address (+)
         effective_address = assign_high_byte(effective_address, high_byte_of_address);
         value = memory_.read(effective_address);
      }

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   auto Processor::x_indirect(ReadOperation const operation) -> Instruction
   {
      // fetch pointer address, increment PC
      Byte pointer_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from the address, add X to it
      std::ignore = memory_.read(pointer_address); // ???
      pointer_address += x_;
      co_await std::suspend_always{};

      // fetch effective address low
      Byte const effective_address_low{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // fetch effective address high
      ++pointer_address;
      Byte const effective_address_high{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // read from effective address
      Word const effective_address{assemble_word(effective_address_high, effective_address_low)};
      auto const value{memory_.read(effective_address)};

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   auto Processor::indirect_y(ReadOperation const operation) -> Instruction
   {
      // fetch pointer address, increment PC
      Byte pointer_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch effective address low
      Byte const effective_address_low{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      ++pointer_address;
      Byte effective_address_high{memory_.read(pointer_address)};
      auto const [low_byte, overflow]{add_with_overflow(effective_address_low, y_)};
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      Word effective_address{assemble_word(effective_address_high, low_byte)};
      Byte value{memory_.read(effective_address)};
      if (overflow)
      {
         ++effective_address_high;
         co_await std::suspend_always{};

         // read from effective address (+)
         effective_address = assign_high_byte(effective_address, effective_address_high);
         value = memory_.read(effective_address);
      }

      std::invoke(operation, this, value);
      co_return std::nullopt;
   }

   auto Processor::accumulator(ModifyOperation const operation) -> Instruction
   {
      // do the operation on the accumulator
      accumulator_ = std::invoke(operation, this, accumulator_);
      co_return std::nullopt;
   }

   auto Processor::absolute(ModifyOperation const operation) -> Instruction
   {
      // fetch low byte of address, increment PC
      Byte const low_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Byte const high_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address
      Word const effective_address{assemble_word(high_byte_of_address, low_byte_of_address)};
      Byte value{memory_.read(effective_address)};
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return std::nullopt;
   }

   auto Processor::zero_page(ModifyOperation const operation) -> Instruction
   {
      // fetch address, increment PC
      Word const address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address
      Byte value{memory_.read(address)};
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(address, value);
      co_return std::nullopt;
   }

   auto Processor::zero_page_indexed(ModifyOperation const operation, Index const index) -> Instruction
   {
      // fetch address, increment PC
      Byte address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from address, add index register to it
      std::ignore = memory_.read(address); // ???
      address += index;
      co_await std::suspend_always{};

      // read from effective address
      Byte value{memory_.read(address)};
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(address, value);
      co_return std::nullopt;
   }

   auto Processor::absolute_indexed(ModifyOperation const operation, Index const index) -> Instruction
   {
      // fetch low byte of address, increment PC
      Byte const low_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Byte high_byte_of_address{memory_.read(program_counter)};
      auto const [low_byte, overflow]{add_with_overflow(low_byte_of_address, index)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      Word effective_address{assemble_word(high_byte_of_address, low_byte)};
      Byte value{memory_.read(effective_address)};
      high_byte_of_address += overflow;
      co_await std::suspend_always{};

      // re-read from effective address
      effective_address = assign_high_byte(effective_address, high_byte_of_address);
      value = memory_.read(effective_address);
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return std::nullopt;
   }

   auto Processor::x_indirect(ModifyOperation const operation) -> Instruction
   {
      // fetch pointer address, increment PC
      Byte pointer_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from the address, add X to it
      std::ignore = memory_.read(pointer_address); // ???
      pointer_address += x_;
      co_await std::suspend_always{};

      // fetch effective address low
      Byte const effective_address_low{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // fetch effective address high
      ++pointer_address;
      Byte const effective_address_high{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // read from effective address
      Word const effective_address{assemble_word(effective_address_high, effective_address_low)};
      auto value{memory_.read(effective_address)};
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return std::nullopt;
   }

   auto Processor::indirect_y(ModifyOperation const operation) -> Instruction
   {
      // fetch pointer address, increment PC
      Byte pointer_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch effective address low
      Byte const effective_address_low{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      ++pointer_address;
      Byte effective_address_high{memory_.read(pointer_address)};
      auto const [low_byte, overflow]{add_with_overflow(effective_address_low, y_)};
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      Word effective_address{assemble_word(effective_address_high, low_byte)};
      Byte value{memory_.read(effective_address)};
      effective_address_high += overflow;
      co_await std::suspend_always{};

      // read from effective address
      effective_address = assign_high_byte(effective_address, effective_address_high);
      value = memory_.read(effective_address);
      co_await std::suspend_always{};

      // write the value back to effective address, and do the operation on it
      memory_.write(effective_address, value);
      value = std::invoke(operation, this, value);
      co_await std::suspend_always{};

      // write the new value to effective address
      memory_.write(effective_address, value);
      co_return std::nullopt;
   }

   auto Processor::absolute(WriteOperation const operation) -> Instruction
   {
      // fetch low byte of address, increment PC
      Byte const low_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, increment PC
      Byte const high_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // write register to effective address
      Word const effective_address{assemble_word(high_byte_of_address, low_byte_of_address)};
      memory_.write(effective_address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   auto Processor::zero_page(WriteOperation const operation) -> Instruction
   {
      // fetch address, increment PC
      Word const address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // write register to effective address
      memory_.write(address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   auto Processor::zero_page_indexed(WriteOperation const operation, Index const index) -> Instruction
   {
      // fetch address, increment PC
      Byte address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from address, add index register to it
      std::ignore = memory_.read(address); // ???
      address += index;
      co_await std::suspend_always{};

      // write to effective address
      memory_.write(address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   auto Processor::absolute_indexed(WriteOperation const operation, Index const index) -> Instruction
   {
      // fetch low byte of address, increment PC
      Byte const low_byte_of_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch high byte of address, add index register to low address byte, increment PC
      Byte high_byte_of_address{memory_.read(program_counter)};
      auto const [low_byte, overflow]{add_with_overflow(low_byte_of_address, index)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from effective address, fix the high byte of effective address
      Word effective_address{assemble_word(high_byte_of_address, low_byte)};
      std::ignore = memory_.read(effective_address);
      high_byte_of_address += overflow;
      co_await std::suspend_always{};

      // write to effective address
      effective_address = assign_high_byte(effective_address, high_byte_of_address);
      memory_.write(effective_address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   auto Processor::x_indirect(WriteOperation const operation) -> Instruction
   {
      // fetch pointer address, increment PC
      Byte pointer_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // read from the address, add X to it
      std::ignore = memory_.read(pointer_address); // ???
      pointer_address += x_;
      co_await std::suspend_always{};

      // fetch effective address low
      Byte const effective_address_low{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // fetch effective address high
      ++pointer_address;
      Byte const effective_address_high{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // write to effective address
      Word const effective_address{assemble_word(effective_address_high, effective_address_low)};
      memory_.write(effective_address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   auto Processor::indirect_y(WriteOperation const operation) -> Instruction
   {
      // fetch pointer address, increment PC
      Byte pointer_address{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch effective address low
      Byte const effective_address_low{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // fetch effective address high, add Y to low byte of effective address
      ++pointer_address;
      Byte effective_address_high{memory_.read(pointer_address)};
      auto const [low_byte, overflow]{add_with_overflow(effective_address_low, y_)};
      co_await std::suspend_always{};

      // read from effective address, fix high byte of effective address
      Word effective_address{assemble_word(effective_address_high, low_byte)};
      std::ignore = memory_.read(effective_address);
      effective_address_high += overflow;
      co_await std::suspend_always{};

      // write to effective address
      effective_address = assign_high_byte(effective_address, effective_address_high);
      memory_.write(effective_address, std::invoke(operation, this));
      co_return std::nullopt;
   }

   // TODO: find what exactly happens here
   auto Processor::RST() -> Instruction
   {
      co_await std::suspend_always{};

      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      --stack_pointer_;
      co_await std::suspend_always{};

      program_counter = assign_low_byte(program_counter, memory_.read(RESET_LOW));
      co_await std::suspend_always{};

      program_counter = assign_high_byte(program_counter, memory_.read(RESET_HIGH));
      co_return std::nullopt;
   }

   auto Processor::BRK() -> Instruction
   {
      // read next instruction byte (and throw it away), increment PC
      std::ignore = memory_.read(program_counter);
      ++program_counter;
      co_await std::suspend_always{};

      // push PCH on stack (with B flag set), decrement S
      change_processor_status_flag(ProcessorStatusFlag::B, true);
      write_to_stack(high_byte(program_counter));
      --stack_pointer_;
      co_await std::suspend_always{};

      // push PCL on stack, decrement S
      write_to_stack(low_byte(program_counter));
      --stack_pointer_;
      co_await std::suspend_always{};

      // push P on stack, decrement S
      write_to_stack(processor_status_);
      --stack_pointer_;
      co_await std::suspend_always{};

      // fetch PCL
      program_counter = assign_low_byte(program_counter, memory_.read(IRQ_LOW));
      co_await std::suspend_always{};

      // fetch PCH
      program_counter = assign_high_byte(program_counter, memory_.read(IRQ_HIGH));

      // ???
      change_processor_status_flag(ProcessorStatusFlag::I, true);
      co_return std::nullopt;
   }

   auto Processor::PHP() -> Instruction
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // push register on stack (with B and _ flag set), decrement S
      change_processor_status_flag(ProcessorStatusFlag::B, true);
      change_processor_status_flag(ProcessorStatusFlag::_, true);
      write_to_stack(processor_status_);
      --stack_pointer_;
      co_return std::nullopt;
   }

   auto Processor::CLC() -> Instruction
   {
      change_processor_status_flag(ProcessorStatusFlag::C, false);
      co_return std::nullopt;
   }

   auto Processor::JSR() -> Instruction
   {
      // fetch low address byte, increment PC
      Byte const low_address_byte{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // internal operation (pre-decrement S?)
      // --stack_pointer_;
      co_await std::suspend_always{};

      // push PCH on stack, decrement S
      write_to_stack(high_byte(program_counter));
      --stack_pointer_;
      co_await std::suspend_always{};

      // push PCL on stack, decrement S
      write_to_stack(low_byte(program_counter));
      --stack_pointer_;
      co_await std::suspend_always{};

      // copy low address byte to PCL, fetch high address byte to PCH
      program_counter = assemble_word(memory_.read(program_counter), low_address_byte);
      co_return std::nullopt;
   }

   auto Processor::PLP() -> Instruction
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // increment S
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull register from stack (with B and _ flag ignored)
      processor_status_ = (processor_status_ & 0b0011'0000) | read_from_stack(); // TODO: make this cleaner
      co_return std::nullopt;
   }

   auto Processor::SEC() -> Instruction
   {
      // set C
      change_processor_status_flag(ProcessorStatusFlag::C, true);
      co_return std::nullopt;
   }

   auto Processor::RTI() -> Instruction
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // increment S
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull P from stack, increment S
      processor_status_ = (processor_status_ & 0b0011'0000) | read_from_stack(); // TODO: make this cleaner
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull PCL from stack, increment S
      program_counter = assign_low_byte(program_counter, read_from_stack());
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull PCH from stack
      program_counter = assign_high_byte(program_counter, read_from_stack());
      co_return std::nullopt;
   }

   auto Processor::PHA() -> Instruction
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // push register on stack, decrement S
      write_to_stack(accumulator_);
      --stack_pointer_;
      co_return std::nullopt;
   }

   auto Processor::JMP_absolute() -> Instruction
   {
      // fetch low address byte, increment PC
      Byte const low_address_byte{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // copy low address byte to PCL, fetch high address byte to PCH
      program_counter = assemble_word(memory_.read(program_counter), low_address_byte);
      co_return std::nullopt;
   }

   auto Processor::CLI() -> Instruction
   {
      // clear I
      change_processor_status_flag(ProcessorStatusFlag::I, false);
      co_return std::nullopt;
   }

   auto Processor::RTS() -> Instruction
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // increment S
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull PCL from stack, increment S
      program_counter = assign_low_byte(program_counter, read_from_stack());
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull PCH from stack
      program_counter = assign_high_byte(program_counter, read_from_stack());
      co_await std::suspend_always{};

      // increment PC
      ++program_counter;
      co_return std::nullopt;
   }

   auto Processor::PLA() -> Instruction
   {
      // read next instruction byte (and throw it away)
      std::ignore = memory_.read(program_counter);
      co_await std::suspend_always{};

      // increment S
      ++stack_pointer_;
      co_await std::suspend_always{};

      // pull register from stack
      update_zero_and_negative_flag(accumulator_ = read_from_stack());
      co_return std::nullopt;
   }

   auto Processor::JMP_indirect() -> Instruction
   {
      // fetch pointer address low, increment PC
      Byte pointer_address_low{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch pointer address high, increment PC
      Byte const pointer_address_high{memory_.read(program_counter)};
      ++program_counter;
      co_await std::suspend_always{};

      // fetch low address to latch
      Word pointer_address{assemble_word(pointer_address_high, pointer_address_low)};
      Byte const low_address{memory_.read(pointer_address)};
      co_await std::suspend_always{};

      // fetch PCH, copy latch to PCL
      ++pointer_address_low;
      pointer_address = assign_low_byte(pointer_address, pointer_address_low);
      program_counter = assemble_word(memory_.read(pointer_address), low_address);
      co_return std::nullopt;
   }

   auto Processor::SEI() -> Instruction
   {
      // set I
      change_processor_status_flag(ProcessorStatusFlag::I, true);
      co_return std::nullopt;
   }

   auto Processor::DEY() -> Instruction
   {
      // decrement Y
      update_zero_and_negative_flag(--y_);
      co_return std::nullopt;
   }

   auto Processor::TXA() -> Instruction
   {
      // transfer X to A
      update_zero_and_negative_flag(accumulator_ = x_);
      co_return std::nullopt;
   }

   auto Processor::TYA() -> Instruction
   {
      // transfer Y to A
      update_zero_and_negative_flag(accumulator_ = y_);
      co_return std::nullopt;
   }

   auto Processor::TXS() -> Instruction
   {
      // transfer X to S
      stack_pointer_ = x_;
      co_return std::nullopt;
   }

   auto Processor::TAY() -> Instruction
   {
      // transfer A to Y
      update_zero_and_negative_flag(y_ = accumulator_);
      co_return std::nullopt;
   }

   auto Processor::TAX() -> Instruction
   {
      // transfer A to X
      update_zero_and_negative_flag(x_ = accumulator_);
      co_return std::nullopt;
   }

   auto Processor::CLV() -> Instruction
   {
      // clear V
      change_processor_status_flag(ProcessorStatusFlag::V, false);
      co_return std::nullopt;
   }

   auto Processor::TSX() -> Instruction
   {
      // transfer S to X
      update_zero_and_negative_flag(x_ = stack_pointer_);
      co_return std::nullopt;
   }

   auto Processor::INY() -> Instruction
   {
      // increment Y
      update_zero_and_negative_flag(++y_);
      co_return std::nullopt;
   }

   auto Processor::DEX() -> Instruction
   {
      // decrement X
      update_zero_and_negative_flag(--x_);
      co_return std::nullopt;
   }

   auto Processor::CLD() -> Instruction
   {
      // clear D
      change_processor_status_flag(ProcessorStatusFlag::D, false);
      co_return std::nullopt;
   }

   auto Processor::INX() -> Instruction
   {
      // increment X
      update_zero_and_negative_flag(++x_);
      co_return std::nullopt;
   }

   auto Processor::NOP() -> Instruction
   {
      co_return std::nullopt;
   }

   auto Processor::SED() -> Instruction
   {
      // set D
      change_processor_status_flag(ProcessorStatusFlag::D, true);
      co_return std::nullopt;
   }

   auto Processor::BPL() const -> bool
   {
      return not processor_status_flag(ProcessorStatusFlag::N);
   }

   auto Processor::BMI() const -> bool
   {
      return processor_status_flag(ProcessorStatusFlag::N);
   }

   auto Processor::BVC() const -> bool
   {
      return not processor_status_flag(ProcessorStatusFlag::V);
   }

   auto Processor::BVS() const -> bool
   {
      return processor_status_flag(ProcessorStatusFlag::V);
   }

   auto Processor::BCC() const -> bool
   {
      return not processor_status_flag(ProcessorStatusFlag::C);
   }

   auto Processor::BCS() const -> bool
   {
      return processor_status_flag(ProcessorStatusFlag::C);
   }

   auto Processor::BNE() const -> bool
   {
      return not processor_status_flag(ProcessorStatusFlag::Z);
   }

   auto Processor::BEQ() const -> bool
   {
      return processor_status_flag(ProcessorStatusFlag::Z);
   }

   auto Processor::ORA(Byte const value) -> void
   {
      // ORA value with accumulator
      update_zero_and_negative_flag(accumulator_ |= value);
   }

   auto Processor::LDA(Byte const value) -> void
   {
      // load value into accumulator
      update_zero_and_negative_flag(accumulator_ = value);
   }

   auto Processor::AND(Byte const value) -> void
   {
      // AND value with accumulator
      update_zero_and_negative_flag(accumulator_ &= value);
   }

   auto Processor::BIT(Byte const value) -> void
   {
      // N set to bit 7 of value
      change_processor_status_flag(ProcessorStatusFlag::N, value & 0b1000'0000);

      // V set to bit 6 of value
      change_processor_status_flag(ProcessorStatusFlag::V, value & 0b0100'0000);

      // Z set if value AND accumulator is zero
      change_processor_status_flag(ProcessorStatusFlag::Z, not(value & accumulator_));
   }

   auto Processor::EOR(Byte const value) -> void
   {
      // EOR value with accumulator
      update_zero_and_negative_flag(accumulator_ ^= value);
   }

   auto Processor::ADC(Byte const value) -> void
   {
      // perform addition
      auto const result{accumulator_ + value + processor_status_flag(ProcessorStatusFlag::C)};

      // set C if there was a carry-out
      change_processor_status_flag(ProcessorStatusFlag::C, result > 0xFF);

      // set V if there was a signed overflow
      change_processor_status_flag(ProcessorStatusFlag::V, (accumulator_ ^ result) & (value ^ result) & 0x80);

      // store result in accumulator
      update_zero_and_negative_flag(accumulator_ = static_cast<Byte>(result));
   }

   auto Processor::LDY(Byte const value) -> void
   {
      // load value into Y register
      update_zero_and_negative_flag(y_ = value);
   }

   auto Processor::LDX(Byte const value) -> void
   {
      // load value into X register
      update_zero_and_negative_flag(x_ = value);
   }

   auto Processor::CPY(Byte const value) -> void
   {
      change_processor_status_flag(ProcessorStatusFlag::C, y_ >= value);
      update_zero_and_negative_flag(y_ - value);
   }

   auto Processor::CMP(Byte const value) -> void
   {
      change_processor_status_flag(ProcessorStatusFlag::C, accumulator_ >= value);
      update_zero_and_negative_flag(accumulator_ - value);
   }

   auto Processor::CPX(Byte const value) -> void
   {
      change_processor_status_flag(ProcessorStatusFlag::C, x_ >= value);
      update_zero_and_negative_flag(x_ - value);
   }

   auto Processor::SBC(Byte const value) -> void
   {
      // perform subtraction
      auto const result{accumulator_ - value - not processor_status_flag(ProcessorStatusFlag::C)};

      // set C if there was no borrow
      change_processor_status_flag(ProcessorStatusFlag::C, result >= 0);

      // set V if there was a signed overflow
      change_processor_status_flag(ProcessorStatusFlag::V, (accumulator_ ^ value) & (accumulator_ ^ result) & 0x80);

      // store result in accumulator
      update_zero_and_negative_flag(accumulator_ = static_cast<Byte>(result));
   }

   auto Processor::ASL(Byte value) -> Byte
   {
      // C set to bit 7 of value before ASL
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0x80);

      // shift value left by 1
      update_zero_and_negative_flag(value <<= 1);
      return value;
   }

   auto Processor::ROL(Byte value) -> Byte
   {
      // save old C
      Byte const old_carry{processor_status_flag(ProcessorStatusFlag::C)};

      // C set to bit 7 of value before ROL
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0b1000'0000);

      // shift value left by 1, set bit 0 to old C
      update_zero_and_negative_flag(value = value << 1 | old_carry);
      return value;
   }

   auto Processor::LSR(Byte value) -> Byte
   {
      // C set to bit 0 of value before LSR
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0b0000'0001);

      // shift value right by 1
      update_zero_and_negative_flag(value >>= 1);
      return value;
   }

   auto Processor::ROR(Byte value) -> Byte
   {
      // save old C
      Byte const old_carry{processor_status_flag(ProcessorStatusFlag::C)};

      // C set to bit 0 of value before ROR
      change_processor_status_flag(ProcessorStatusFlag::C, value & 0b0000'0001);

      // shift value right by 1, set bit 7 to old C
      update_zero_and_negative_flag(value = value >> 1 | old_carry << 7);
      return value;
   }

   auto Processor::DEC(Byte value) -> Byte
   {
      // decrement value
      update_zero_and_negative_flag(--value);
      return value;
   }

   auto Processor::INC(Byte value) -> Byte
   {
      // increment value
      update_zero_and_negative_flag(++value);
      return value;
   }

   auto Processor::STA() -> Byte
   {
      return accumulator_;
   }

   auto Processor::STY() -> Byte
   {
      return y_;
   }

   auto Processor::STX() -> Byte
   {
      return x_;
   }

   auto Processor::instruction_from_opcode(Opcode const opcode) -> Instruction
   {
      auto const throw_unsupported_opcode{
         [this, opcode] [[noreturn]] (std::source_location const& location = std::source_location::current()) -> void
         {
            throw UnsupportedOpcode{
               static_cast<decltype(program_counter)>(program_counter - 1), static_cast<std::underlying_type_t<Opcode>>(opcode),
               location
            };
         }
      };

      switch (opcode)
      {
         case Opcode::BRK_IMPLIED:
            return BRK();

         case Opcode::ORA_X_INDIRECT:
            return x_indirect(&Processor::ORA);

         case Opcode::JAM_IMPLIED_02:
            [[fallthrough]];

         case Opcode::SLO_X_INDIRECT:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_04:
            throw_unsupported_opcode();

         case Opcode::ORA_ZERO_PAGE:
            return zero_page(&Processor::ORA);

         case Opcode::ASL_ZERO_PAGE:
            return zero_page(&Processor::ASL);

         case Opcode::SLO_ZERO_PAGE:
            throw_unsupported_opcode();

         case Opcode::PHP_IMPLIED:
            return PHP();

         case Opcode::ORA_IMMEDIATE:
            return immediate(&Processor::ORA);

         case Opcode::ASL_ACCUMULATOR:
            return accumulator(&Processor::ASL);

         case Opcode::ANC_IMMEDIATE_0B:
            [[fallthrough]];

         case Opcode::NOP_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::ORA_ABSOLUTE:
            return absolute(&Processor::ORA);

         case Opcode::ASL_ABSOLUTE:
            return absolute(&Processor::ASL);

         case Opcode::SLO_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::BPL_RELATIVE:
            return relative(&Processor::BPL);

         case Opcode::ORA_INDIRECT_Y:
            return indirect_y(&Processor::ORA);

         case Opcode::JAM_IMPLIED_12:
            [[fallthrough]];

         case Opcode::SLO_INDIRECT_Y:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_X_14:
            throw_unsupported_opcode();

         case Opcode::ORA_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ORA, x_);

         case Opcode::ASL_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ASL, x_);

         case Opcode::SLO_ZERO_PAGE_X:
            throw_unsupported_opcode();

         case Opcode::CLC_IMPLIED:
            return CLC();

         case Opcode::ORA_ABSOLUTE_Y:
            return absolute_indexed(&Processor::ORA, y_);

         case Opcode::NOP_IMPLIED_1A:
            [[fallthrough]];

         case Opcode::SLO_ABSOLUTE_Y:
            [[fallthrough]];

         case Opcode::NOP_ABSOLUTE_X_1C:
            throw_unsupported_opcode();

         case Opcode::ORA_ABSOLUTE_X:
            return absolute_indexed(&Processor::ORA, x_);

         case Opcode::ASL_ABSOLUTE_X:
            return absolute_indexed(&Processor::ASL, x_);

         case Opcode::SLO_ABSOLUTE_X:
            throw_unsupported_opcode();

         case Opcode::JSR_ABSOLUTE:
            return JSR();

         case Opcode::AND_X_INDIRECT:
            return x_indirect(&Processor::AND);

         case Opcode::JAM_IMPLIED_22:
            [[fallthrough]];

         case Opcode::RLA_X_INDIRECT:
            throw_unsupported_opcode();

         case Opcode::BIT_ZERO_PAGE:
            return zero_page(&Processor::BIT);

         case Opcode::AND_ZERO_PAGE:
            return zero_page(&Processor::AND);

         case Opcode::ROL_ZERO_PAGE:
            return zero_page(&Processor::ROL);

         case Opcode::RLA_ZERO_PAGE:
            throw_unsupported_opcode();

         case Opcode::PLP_IMPLIED:
            return PLP();

         case Opcode::AND_IMMEDIATE:
            return immediate(&Processor::AND);

         case Opcode::ROL_ACCUMULATOR:
            return accumulator(&Processor::ROL);

         case Opcode::ANC_IMMEDIATE_2B:
            throw_unsupported_opcode();

         case Opcode::BIT_ABSOLUTE:
            return absolute(&Processor::BIT);

         case Opcode::AND_ABSOLUTE:
            return absolute(&Processor::AND);

         case Opcode::ROL_ABSOLUTE:
            return absolute(&Processor::ROL);

         case Opcode::RLA_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::BMI_RELATIVE:
            return relative(&Processor::BMI);

         case Opcode::AND_INDIRECT_Y:
            return indirect_y(&Processor::AND);

         case Opcode::JAM_IMPLIED_32:
            [[fallthrough]];

         case Opcode::RLA_INDIRECT_Y:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_X_34:
            throw_unsupported_opcode();

         case Opcode::AND_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::AND, x_);

         case Opcode::ROL_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ROL, x_);

         case Opcode::RLA_ZERO_PAGE_X:
            throw_unsupported_opcode();

         case Opcode::SEC_IMPLIED:
            return SEC();

         case Opcode::AND_ABSOLUTE_Y:
            return absolute_indexed(&Processor::AND, y_);

         case Opcode::NOP_IMPLIED_3A:
            [[fallthrough]];

         case Opcode::RLA_ABSOLUTE_Y:
            [[fallthrough]];

         case Opcode::NOP_ABSOLUTE_X_3C:
            throw_unsupported_opcode();

         case Opcode::AND_ABSOLUTE_X:
            return absolute_indexed(&Processor::AND, x_);

         case Opcode::ROL_ABSOLUTE_X:
            return absolute_indexed(&Processor::ROL, x_);

         case Opcode::RLA_ABSOLUTE_X:
            throw_unsupported_opcode();

         case Opcode::RTI_IMPLIED:
            return RTI();

         case Opcode::EOR_X_INDIRECT:
            return x_indirect(&Processor::EOR);

         case Opcode::JAM_IMPLIED_42:
            [[fallthrough]];

         case Opcode::SRE_X_INDIRECT:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_44:
            throw_unsupported_opcode();

         case Opcode::EOR_ZERO_PAGE:
            return zero_page(&Processor::EOR);

         case Opcode::LSR_ZERO_PAGE:
            return zero_page(&Processor::LSR);

         case Opcode::SRE_ZERO_PAGE:
            throw_unsupported_opcode();

         case Opcode::PHA_IMPLIED:
            return PHA();

         case Opcode::EOR_IMMEDIATE:
            return immediate(&Processor::EOR);

         case Opcode::LSR_ACCUMULATOR:
            return accumulator(&Processor::LSR);

         case Opcode::ALR_IMMEDIATE_4B:
            throw_unsupported_opcode();

         case Opcode::JMP_ABSOLUTE:
            return JMP_absolute();

         case Opcode::EOR_ABSOLUTE:
            return absolute(&Processor::EOR);

         case Opcode::LSR_ABSOLUTE:
            return absolute(&Processor::LSR);

         case Opcode::SRE_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::BVC_RELATIVE:
            return relative(&Processor::BVC);

         case Opcode::EOR_INDIRECT_Y:
            return indirect_y(&Processor::EOR);

         case Opcode::JAM_IMPLIED_52:
            [[fallthrough]];

         case Opcode::SRE_INDIRECT_Y:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_X_54:
            throw_unsupported_opcode();

         case Opcode::EOR_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::EOR, x_);

         case Opcode::LSR_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::LSR, x_);

         case Opcode::SRE_ZERO_PAGE_X:
            throw_unsupported_opcode();

         case Opcode::CLI_IMPLIED:
            return CLI();

         case Opcode::EOR_ABSOLUTE_Y:
            return absolute_indexed(&Processor::EOR, y_);

         case Opcode::NOP_IMPLIED_5A:
            [[fallthrough]];

         case Opcode::SRE_ABSOLUTE_Y:
            [[fallthrough]];

         case Opcode::NOP_ABSOLUTE_X_5C:
            throw_unsupported_opcode();

         case Opcode::EOR_ABSOLUTE_X:
            return absolute_indexed(&Processor::EOR, x_);

         case Opcode::LSR_ABSOLUTE_X:
            return absolute_indexed(&Processor::LSR, x_);

         case Opcode::SRE_ABSOLUTE_X:
            throw_unsupported_opcode();

         case Opcode::RTS_IMPLIED:
            return RTS();

         case Opcode::ADC_X_INDIRECT:
            return x_indirect(&Processor::ADC);

         case Opcode::JAM_IMPLIED_62:
            [[fallthrough]];

         case Opcode::RRA_X_INDIRECT:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_64:
            throw_unsupported_opcode();

         case Opcode::ADC_ZERO_PAGE:
            return zero_page(&Processor::ADC);

         case Opcode::ROR_ZERO_PAGE:
            return zero_page(&Processor::ROR);

         case Opcode::RRA_ZERO_PAGE:
            throw_unsupported_opcode();

         case Opcode::PLA_IMPLIED:
            return PLA();

         case Opcode::ADC_IMMEDIATE:
            return immediate(&Processor::ADC);

         case Opcode::ROR_ACCUMULATOR:
            return accumulator(&Processor::ROR);

         case Opcode::ARR_IMMEDIATE:
            throw_unsupported_opcode();

         case Opcode::JMP_INDIRECT:
            return JMP_indirect();

         case Opcode::ADC_ABSOLUTE:
            return absolute(&Processor::ADC);

         case Opcode::ROR_ABSOLUTE:
            return absolute(&Processor::ROR);

         case Opcode::RRA_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::BVS_RELATIVE:
            return relative(&Processor::BVS);

         case Opcode::ADC_INDIRECT_Y:
            return indirect_y(&Processor::ADC);

         case Opcode::JAM_IMPLIED_72:
            [[fallthrough]];

         case Opcode::RRA_INDIRECT_Y:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_X_74:
            throw_unsupported_opcode();

         case Opcode::ADC_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ADC, x_);

         case Opcode::ROR_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::ROR, x_);

         case Opcode::RRA_ZERO_PAGE_X:
            throw_unsupported_opcode();

         case Opcode::SEI_IMPLIED:
            return SEI();

         case Opcode::ADC_ABSOLUTE_Y:
            return absolute_indexed(&Processor::ADC, y_);

         case Opcode::NOP_IMPLIED_7A:
            [[fallthrough]];

         case Opcode::RRA_ABSOLUTE_Y:
            [[fallthrough]];

         case Opcode::NOP_ABSOLUTE_X_7C:
            throw_unsupported_opcode();

         case Opcode::ADC_ABSOLUTE_X:
            return absolute_indexed(&Processor::ADC, x_);

         case Opcode::ROR_ABSOLUTE_X:
            return absolute_indexed(&Processor::ROR, x_);

         case Opcode::RRA_ABSOLUTE_X:
            [[fallthrough]];

         case Opcode::NOP_IMMEDIATE_80:
            throw_unsupported_opcode();

         case Opcode::STA_X_INDIRECT:
            return x_indirect(&Processor::STA);

         case Opcode::NOP_IMMEDIATE_82:
            [[fallthrough]];

         case Opcode::SAX_X_INDIRECT:
            throw_unsupported_opcode();

         case Opcode::STY_ZERO_PAGE:
            return zero_page(&Processor::STY);

         case Opcode::STA_ZERO_PAGE:
            return zero_page(&Processor::STA);

         case Opcode::STX_ZERO_PAGE:
            return zero_page(&Processor::STX);

         case Opcode::SAX_ZERO_PAGE:
            throw_unsupported_opcode();

         case Opcode::DEY_IMPLIED:
            return DEY();

         case Opcode::NOP_IMMEDIATE_89:
            throw_unsupported_opcode();

         case Opcode::TXA_IMPLIED:
            return TXA();

         case Opcode::ANE_IMMEDIATE:
            throw_unsupported_opcode();

         case Opcode::STY_ABSOLUTE:
            return absolute(&Processor::STY);

         case Opcode::STA_ABSOLUTE:
            return absolute(&Processor::STA);

         case Opcode::STX_ABSOLUTE:
            return absolute(&Processor::STX);

         case Opcode::SAX_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::BCC_RELATIVE:
            return relative(&Processor::BCC);

         case Opcode::STA_INDIRECT_Y:
            return indirect_y(&Processor::STA);

         case Opcode::JAM_IMPLIED_92:
            [[fallthrough]];

         case Opcode::SHA_INDIRECT_Y:
            throw_unsupported_opcode();

         case Opcode::STY_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::STY, x_);

         case Opcode::STA_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::STA, x_);

         case Opcode::STX_ZERO_PAGE_Y:
            return zero_page_indexed(&Processor::STX, y_);

         case Opcode::SAX_ZERO_PAGE_Y:
            throw_unsupported_opcode();

         case Opcode::TYA_IMPLIED:
            return TYA();

         case Opcode::STA_ABSOLUTE_Y:
            return absolute_indexed(&Processor::STA, y_);

         case Opcode::TXS_IMPLIED:
            return TXS();

         case Opcode::TAS_ABSOLUTE_Y:
            [[fallthrough]];

         case Opcode::SHY_ABSOLUTE_X:
            throw_unsupported_opcode();

         case Opcode::STA_ABSOLUTE_X:
            return absolute_indexed(&Processor::STA, x_);

         case Opcode::SHX_ABSOLUTE_Y:
            [[fallthrough]];

         case Opcode::SHA_ABSOLUTE_Y:
            throw_unsupported_opcode();

         case Opcode::LDY_IMMEDIATE:
            return immediate(&Processor::LDY);

         case Opcode::LDA_X_INDIRECT:
            return x_indirect(&Processor::LDA);

         case Opcode::LDX_IMMEDIATE:
            return immediate(&Processor::LDX);

         case Opcode::LAX_X_INDIRECT:
            throw_unsupported_opcode();

         case Opcode::LDY_ZERO_PAGE:
            return zero_page(&Processor::LDY);

         case Opcode::LDA_ZERO_PAGE:
            return zero_page(&Processor::LDA);

         case Opcode::LDX_ZERO_PAGE:
            return zero_page(&Processor::LDX);

         case Opcode::LAX_ZERO_PAGE:
            throw_unsupported_opcode();

         case Opcode::TAY_IMPLIED:
            return TAY();

         case Opcode::LDA_IMMEDIATE:
            return immediate(&Processor::LDA);

         case Opcode::TAX_IMPLIED:
            return TAX();

         case Opcode::LXA_IMMEDIATE:
            throw_unsupported_opcode();

         case Opcode::LDY_ABSOLUTE:
            return absolute(&Processor::LDY);

         case Opcode::LDA_ABSOLUTE:
            return absolute(&Processor::LDA);

         case Opcode::LDX_ABSOLUTE:
            return absolute(&Processor::LDX);

         case Opcode::LAX_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::BCS_RELATIVE:
            return relative(&Processor::BCS);

         case Opcode::LDA_INDIRECT_Y:
            return indirect_y(&Processor::LDA);

         case Opcode::JAM_IMPLIED_B2:
            [[fallthrough]];

         case Opcode::LAX_INDIRECT_Y:
            throw_unsupported_opcode();

         case Opcode::LDY_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::LDY, x_);

         case Opcode::LDA_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::LDA, x_);

         case Opcode::LDX_ZERO_PAGE_Y:
            return zero_page_indexed(&Processor::LDX, y_);

         case Opcode::LAX_ZERO_PAGE_Y:
            throw_unsupported_opcode();

         case Opcode::CLV_IMPLIED:
            return CLV();

         case Opcode::LDA_ABSOLUTE_Y:
            return absolute_indexed(&Processor::LDA, y_);

         case Opcode::TSX_IMPLIED:
            return TSX();

         case Opcode::LAS_ABSOLUTE_Y:
            throw_unsupported_opcode();

         case Opcode::LDY_ABSOLUTE_X:
            return absolute_indexed(&Processor::LDY, x_);

         case Opcode::LDA_ABSOLUTE_X:
            return absolute_indexed(&Processor::LDA, x_);

         case Opcode::LDX_ABSOLUTE_Y:
            return absolute_indexed(&Processor::LDX, y_);

         case Opcode::LAX_ABSOLUTE_Y:
            throw_unsupported_opcode();

         case Opcode::CPY_IMMEDIATE:
            return immediate(&Processor::CPY);

         case Opcode::CMP_X_INDIRECT:
            return x_indirect(&Processor::CMP);

         case Opcode::NOP_IMMEDIATE_C2:
            [[fallthrough]];

         case Opcode::DCP_X_INDIRECT:
            throw_unsupported_opcode();

         case Opcode::CPY_ZERO_PAGE:
            return zero_page(&Processor::CPY);

         case Opcode::CMP_ZERO_PAGE:
            return zero_page(&Processor::CMP);

         case Opcode::DEC_ZERO_PAGE:
            return zero_page(&Processor::DEC);

         case Opcode::DCP_ZERO_PAGE:
            throw_unsupported_opcode();

         case Opcode::INY_IMPLIED:
            return INY();

         case Opcode::CMP_IMMEDIATE:
            return immediate(&Processor::CMP);

         case Opcode::DEX_IMPLIED:
            return DEX();

         case Opcode::SBX_IMMEDIATE:
            throw_unsupported_opcode();

         case Opcode::CPY_ABSOLUTE:
            return absolute(&Processor::CPY);

         case Opcode::CMP_ABSOLUTE:
            return absolute(&Processor::CMP);

         case Opcode::DEC_ABSOLUTE:
            return absolute(&Processor::DEC);

         case Opcode::DCP_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::BNE_RELATIVE:
            return relative(&Processor::BNE);

         case Opcode::CMP_INDIRECT_Y:
            return indirect_y(&Processor::CMP);

         case Opcode::JAM_IMPLIED_D2:
            [[fallthrough]];

         case Opcode::DCP_INDIRECT_Y:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_X_D4:
            throw_unsupported_opcode();

         case Opcode::CMP_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::CMP, x_);

         case Opcode::DEC_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::DEC, x_);

         case Opcode::DCP_ZERO_PAGE_X:
            throw_unsupported_opcode();

         case Opcode::CLD_IMPLIED:
            return CLD();

         case Opcode::CMP_ABSOLUTE_Y:
            return absolute_indexed(&Processor::CMP, y_);

         case Opcode::NOP_IMPLIED_DA:
            [[fallthrough]];

         case Opcode::DCP_ABSOLUTE_Y:
            [[fallthrough]];

         case Opcode::NOP_ABSOLUTE_X_DC:
            throw_unsupported_opcode();

         case Opcode::CMP_ABSOLUTE_X:
            return absolute_indexed(&Processor::CMP, x_);

         case Opcode::DEC_ABSOLUTE_X:
            return absolute_indexed(&Processor::DEC, x_);

         case Opcode::DCP_ABSOLUTE_X:
            throw_unsupported_opcode();

         case Opcode::CPX_IMMEDIATE:
            return immediate(&Processor::CPX);

         case Opcode::SBC_X_INDIRECT:
            return x_indirect(&Processor::SBC);

         case Opcode::NOP_IMMEDIATE_E2:
            [[fallthrough]];

         case Opcode::ISC_X_INDIRECT:
            throw_unsupported_opcode();

         case Opcode::CPX_ZERO_PAGE:
            return zero_page(&Processor::CPX);

         case Opcode::SBC_ZERO_PAGE:
            return zero_page(&Processor::SBC);

         case Opcode::INC_ZERO_PAGE:
            return zero_page(&Processor::INC);

         case Opcode::ISC_ZERO_PAGE:
            throw_unsupported_opcode();

         case Opcode::INX_IMPLIED:
            return INX();

         case Opcode::SBC_IMMEDIATE_E9:
            return immediate(&Processor::SBC);

         case Opcode::NOP_IMPLIED_EA:
            return NOP();

         case Opcode::SBC_IMMEDIATE_EB:
            throw_unsupported_opcode();

         case Opcode::CPX_ABSOLUTE:
            return absolute(&Processor::CPX);

         case Opcode::SBC_ABSOLUTE:
            return absolute(&Processor::SBC);

         case Opcode::INC_ABSOLUTE:
            return absolute(&Processor::INC);

         case Opcode::ISC_ABSOLUTE:
            throw_unsupported_opcode();

         case Opcode::BEQ_RELATIVE:
            return relative(&Processor::BEQ);

         case Opcode::SBC_INDIRECT_Y:
            return indirect_y(&Processor::SBC);

         case Opcode::JAM_IMPLIED_F2:
            [[fallthrough]];

         case Opcode::ISC_INDIRECT_Y:
            [[fallthrough]];

         case Opcode::NOP_ZERO_PAGE_X_F4:
            throw_unsupported_opcode();

         case Opcode::SBC_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::SBC, x_);

         case Opcode::INC_ZERO_PAGE_X:
            return zero_page_indexed(&Processor::INC, x_);

         case Opcode::ISC_ZERO_PAGE_X:
            throw_unsupported_opcode();

         case Opcode::SED_IMPLIED:
            return SED();

         case Opcode::SBC_ABSOLUTE_Y:
            return absolute_indexed(&Processor::SBC, y_);

         case Opcode::NOP_IMPLIED_FA:
            [[fallthrough]];

         case Opcode::ISC_ABSOLUTE_Y:
            [[fallthrough]];

         case Opcode::NOP_ABSOLUTE_X_FC:
            throw_unsupported_opcode();

         case Opcode::SBC_ABSOLUTE_X:
            return absolute_indexed(&Processor::SBC, x_);

         case Opcode::INC_ABSOLUTE_X:
            return absolute_indexed(&Processor::INC, x_);

         case Opcode::ISC_ABSOLUTE_X:
            [[fallthrough]];

         default:
            throw_unsupported_opcode();
      }
   }

   auto Processor::change_processor_status_flag(ProcessorStatusFlag const flag, bool const set) -> void
   {
      auto const underlying_flag{static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag)};
      set ? processor_status_ |= underlying_flag : processor_status_ &= ~underlying_flag;
   }

   auto Processor::processor_status_flag(ProcessorStatusFlag flag) const -> bool
   {
      auto const underlying_flag{static_cast<std::underlying_type_t<ProcessorStatusFlag>>(flag)};
      return processor_status_ & underlying_flag;
   }

   auto Processor::update_zero_and_negative_flag(Byte const value) -> void
   {
      change_processor_status_flag(ProcessorStatusFlag::Z, not value);
      change_processor_status_flag(ProcessorStatusFlag::N, value & 0b1000'0000);
   }

   auto Processor::write_to_stack(Byte const value) const -> void
   {
      memory_.write(0x01'00 + stack_pointer_, value);
   }

   auto Processor::read_from_stack() const -> Byte
   {
      return memory_.read(0x01'00 + stack_pointer_);
   }

   constexpr auto Processor::low_byte(Word const source) -> Byte
   {
      return static_cast<Byte>(source);
   }

   constexpr auto Processor::high_byte(Word const source) -> Byte
   {
      return source >> 8;
   }

   constexpr auto Processor::assemble_word(Byte const high, Byte const low) -> Word
   {
      return high << 8 | low;
   }

   constexpr auto Processor::assign_low_byte(Word const target, Byte const value) -> Word
   {
      return assemble_word(high_byte(target), value);
   }

   constexpr auto Processor::assign_high_byte(Word const target, Byte const value) -> Word
   {
      return assemble_word(value, low_byte(target));
   }

   constexpr auto Processor::add_with_overflow(Byte const left, Byte const right) -> std::pair<Byte, bool>
   {
      auto const result{static_cast<Byte>(left + right)};
      return {result, result < left};
   }

   constexpr auto Processor::add_with_overflow(Byte const left, SignedByte const right) -> std::pair<Byte, SignedByte>
   {
      auto const result{static_cast<Byte>(left + right)};
      auto const overflow{static_cast<SignedByte>(((left ^ result) & (static_cast<Byte>(right) ^ result)) >> 7)};

      return {result, overflow};
   }
}