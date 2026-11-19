#include <doctest/doctest.h>

#include "chip8/emulator.hpp"

#include <cstdint>
#include <initializer_list>
#include <vector>

namespace {

using chip8::Emulator;
using chip8::LoadError;

std::vector<std::uint8_t> encode(std::initializer_list<std::uint16_t> opcodes) {
  std::vector<std::uint8_t> bytes;
  bytes.reserve(opcodes.size() * 2U);
  for (const std::uint16_t opcode : opcodes) {
    bytes.push_back(static_cast<std::uint8_t>((opcode >> 8U) & 0xFFU));
    bytes.push_back(static_cast<std::uint8_t>(opcode & 0xFFU));
  }
  return bytes;
}

void load(Emulator& emulator, std::initializer_list<std::uint16_t> opcodes) {
  const auto rom = encode(opcodes);
  REQUIRE(emulator.load_rom(rom) == LoadError::none);
}

} // namespace

TEST_CASE("reset leaves a clean machine") {
  Emulator emulator;
  CHECK(emulator.pc() == chip8::kProgramStart);
  CHECK(emulator.i_register() == 0);
  CHECK(emulator.delay_timer() == 0);
  CHECK(emulator.sound_timer() == 0);
  CHECK(emulator.cycles() == 0);
  CHECK(emulator.unsupported_opcodes() == 0);
  CHECK_FALSE(emulator.waiting_for_key());
  CHECK_FALSE(emulator.pixel(0, 0));
  CHECK_FALSE(emulator.key_down(0x0F));
}

TEST_CASE("font is loaded at 0x50") {
  Emulator emulator;
  CHECK(emulator.peek(chip8::kFontStart) == 0xF0);
  CHECK(emulator.peek(static_cast<std::uint16_t>(chip8::kFontStart + 4U)) == 0xF0);
}

TEST_CASE("load_rom rejects empty and oversized data") {
  Emulator emulator;

  const std::vector<std::uint8_t> empty;
  CHECK(emulator.load_rom(empty) == LoadError::empty);
  CHECK(emulator.load_rom(nullptr, 0) == LoadError::empty);

  const std::vector<std::uint8_t> oversized(chip8::kMaxRomSize + 1U, 0x00U);
  CHECK(emulator.load_rom(oversized) == LoadError::too_large);
}

TEST_CASE("load_rom resets state from a previous program") {
  Emulator emulator;
  load(emulator, {0x6011, 0x7001});
  emulator.step();
  emulator.step();
  CHECK(emulator.v(0) == 0x12);

  load(emulator, {0x6042});
  CHECK(emulator.v(0) == 0x00);
  CHECK(emulator.pc() == chip8::kProgramStart);
  CHECK(emulator.cycles() == 0);
}

TEST_CASE("register load and add wrap without touching VF") {
  Emulator emulator;
  load(emulator, {0x60FF, 0x7002});
  emulator.step();
  CHECK(emulator.v(0) == 0xFF);

  emulator.step();
  CHECK(emulator.v(0) == 0x01);
  CHECK(emulator.v(0x0F) == 0x00);
}

TEST_CASE("1NNN jumps and 2NNN/00EE call and return") {
  Emulator emulator;
  load(emulator, {0x2206, 0x6001, 0x6102, 0x00EE});

  emulator.step(); // call 0x206
  CHECK(emulator.pc() == 0x0206);

  emulator.step(); // return
  CHECK(emulator.pc() == 0x0202);

  emulator.step(); // V0 = 1
  CHECK(emulator.v(0) == 0x01);

  emulator.step(); // V1 = 2
  CHECK(emulator.v(1) == 0x02);
  CHECK(emulator.pc() == 0x0206);
}

TEST_CASE("conditional skips advance by two bytes") {
  Emulator emulator;
  load(emulator, {0x6005, 0x3005, 0x6101, 0x6202, 0x4004});

  emulator.step(); // V0 = 5
  emulator.step(); // 3XNN equal, skip the next instruction
  CHECK(emulator.pc() == 0x0206);

  emulator.step(); // V2 = 2
  CHECK(emulator.v(2) == 0x02);
  CHECK(emulator.pc() == 0x0208);

  emulator.step(); // 4XNN: V0 != 4, skip
  CHECK(emulator.pc() == 0x020C);
  CHECK(emulator.v(1) == 0x00);
}

TEST_CASE("8XY4 sets the carry flag") {
  Emulator emulator;
  load(emulator, {0x60FF, 0x6102, 0x8014});
  emulator.step();
  emulator.step();
  emulator.step();

  CHECK(emulator.v(0) == 0x01);
  CHECK(emulator.v(0x0F) == 0x01);
}

TEST_CASE("8XY5 sets the no-borrow flag") {
  Emulator emulator;
  load(emulator, {0x6002, 0x6105, 0x8015});
  emulator.step();
  emulator.step();
  emulator.step();

  CHECK(emulator.v(0) == 0xFD);
  CHECK(emulator.v(0x0F) == 0x00);
}

TEST_CASE("8XY6 and 8XYE shift VX in place") {
  Emulator emulator;
  load(emulator, {0x6003, 0x8006, 0x6101, 0x811E});
  emulator.step(); // V0 = 3
  emulator.step(); // V0 = 1, VF = 1
  CHECK(emulator.v(0) == 0x01);
  CHECK(emulator.v(0x0F) == 0x01);

  emulator.step(); // V1 = 1
  emulator.step(); // V1 = 2, VF = 0
  CHECK(emulator.v(1) == 0x02);
  CHECK(emulator.v(0x0F) == 0x00);
}

TEST_CASE("DXYN draws, reports collision, and erases") {
  Emulator emulator;
  // The five opcodes occupy 0x200..0x209 and set I = 0x20A. The two appended
  // bytes are sprite data, never fetched as opcodes: 0x81 is one row with bits
  // 0 and 7, and 0x00 pads the second byte since DXYN reads only one row.
  auto rom = encode({0xA20A, 0x6000, 0x6100, 0xD011, 0xD011});
  rom.push_back(0x81);
  rom.push_back(0x00);
  REQUIRE(emulator.load_rom(rom) == LoadError::none);

  emulator.step(); // I = 0x20A
  emulator.step(); // V0 = 0
  emulator.step(); // V1 = 0
  emulator.step(); // first draw

  CHECK(emulator.pixel(0, 0));
  CHECK(emulator.pixel(7, 0));
  CHECK_FALSE(emulator.pixel(1, 0));
  CHECK(emulator.v(0x0F) == 0x00);

  emulator.step(); // draw the same sprite again
  CHECK(emulator.v(0x0F) == 0x01);
  CHECK_FALSE(emulator.pixel(0, 0));
  CHECK_FALSE(emulator.pixel(7, 0));
}

TEST_CASE("FX33 stores binary-coded decimal") {
  Emulator emulator;
  load(emulator, {0xA300, 0x60EA, 0xF033});
  emulator.step();
  emulator.step();
  emulator.step();

  CHECK(emulator.peek(0x0300) == 0x02);
  CHECK(emulator.peek(0x0301) == 0x03);
  CHECK(emulator.peek(0x0302) == 0x04);
}

TEST_CASE("FX55 and FX65 round-trip and leave I unchanged") {
  Emulator emulator;
  load(emulator, {0xA300, 0x6001, 0x6102, 0x6203, 0xF255, 0xF265});
  emulator.step();
  emulator.step();
  emulator.step();
  emulator.step();
  emulator.step(); // store V0..V2
  emulator.step(); // load V0..V2

  CHECK(emulator.peek(0x0300) == 0x01);
  CHECK(emulator.peek(0x0301) == 0x02);
  CHECK(emulator.peek(0x0302) == 0x03);
  CHECK(emulator.i_register() == 0x0300);
}

TEST_CASE("EX9E and EXA1 test the keypad") {
  Emulator emulator;

  SUBCASE("EX9E does not skip when the key is up") {
    load(emulator, {0x6005, 0xE09E, 0x6101});
    emulator.step(); // V0 = 5
    emulator.step(); // key 5 is up
    CHECK(emulator.pc() == 0x0204);

    emulator.step(); // V1 = 1
    CHECK(emulator.v(1) == 0x01);
  }

  SUBCASE("EX9E skips when the key is down") {
    load(emulator, {0x6005, 0xE09E, 0x6101});
    emulator.set_key(5, true);
    emulator.step(); // V0 = 5
    emulator.step(); // key 5 is down, skip
    CHECK(emulator.pc() == 0x0206);
    CHECK(emulator.v(1) == 0x00);
  }

  SUBCASE("EXA1 skips when the key is up") {
    load(emulator, {0x6005, 0xE0A1, 0x6101});
    emulator.step(); // V0 = 5
    emulator.step(); // key 5 is up, skip
    CHECK(emulator.pc() == 0x0206);
    CHECK(emulator.v(1) == 0x00);
  }
}

TEST_CASE("FX0A blocks until a key is pressed") {
  Emulator emulator;
  load(emulator, {0xF30A, 0x6001});

  emulator.step();
  CHECK(emulator.waiting_for_key());

  const auto pc_while_waiting = emulator.pc();
  emulator.step();
  CHECK(emulator.pc() == pc_while_waiting);
  CHECK(emulator.v(3) == 0x00);

  emulator.set_key(0x0B, true);
  CHECK_FALSE(emulator.waiting_for_key());
  CHECK(emulator.v(3) == 0x0B);

  emulator.step();
  CHECK(emulator.v(0) == 0x01);
}

TEST_CASE("CXNN is masked by the immediate byte") {
  Emulator emulator;
  load(emulator, {0xC00F});
  emulator.step();
  CHECK((emulator.v(0) & 0xF0U) == 0x00U);
}

TEST_CASE("timers decrement to zero and stop") {
  Emulator emulator;
  load(emulator, {0x6002, 0xF015, 0xF018});
  emulator.step();
  emulator.step();
  emulator.step();

  CHECK(emulator.delay_timer() == 0x02);
  CHECK(emulator.sound_timer() == 0x02);

  emulator.tick_timers();
  emulator.tick_timers();
  emulator.tick_timers();

  CHECK(emulator.delay_timer() == 0x00);
  CHECK(emulator.sound_timer() == 0x00);
}

TEST_CASE("unsupported opcodes are counted, not executed") {
  Emulator emulator;
  load(emulator, {0x0123});
  emulator.step();
  CHECK(emulator.unsupported_opcodes() == 1);
  CHECK(emulator.pc() == 0x0202);
}
