#include "chip8/emulator.hpp"

#include <algorithm>
#include <cassert>

namespace chip8 {
namespace {

// Standard 4x5 hexadecimal glyphs, 16 glyphs of 5 bytes each.
constexpr std::array<std::uint8_t, 80> kFontSet = {
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
    0x20, 0x60, 0x20, 0x20, 0x70, // 1
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F
};

constexpr std::size_t kFontGlyphBytes = 5;
constexpr std::uint32_t kInitialSeed = 0x5EEDU;

} // namespace

Emulator::Emulator() {
  reset();
}

void Emulator::reset() {
  memory_.fill(0);
  v_.fill(0);
  stack_.fill(0);
  display_.fill(0);
  keys_.fill(false);

  i_ = 0;
  pc_ = kProgramStart;
  sp_ = 0;
  delay_timer_ = 0;
  sound_timer_ = 0;
  waiting_key_.reset();
  cycles_ = 0;
  unsupported_opcodes_ = 0;
  rng_.seed(kInitialSeed);

  load_font();
}

void Emulator::load_font() {
  std::copy(kFontSet.begin(), kFontSet.end(),
            memory_.begin() + static_cast<std::ptrdiff_t>(kFontStart));
}

LoadError Emulator::load_rom(const std::uint8_t* data, std::size_t size) {
  if (data == nullptr || size == 0) {
    return LoadError::empty;
  }
  if (size > kMaxRomSize) {
    return LoadError::too_large;
  }

  reset();
  std::copy_n(data, size, memory_.begin() + static_cast<std::ptrdiff_t>(kProgramStart));
  return LoadError::none;
}

std::uint8_t Emulator::peek(std::uint16_t address) const {
  assert(static_cast<std::size_t>(address) < kMemorySize);
  return memory_[address];
}

std::uint16_t Emulator::fetch_opcode() const {
  const auto high = static_cast<std::uint16_t>(peek(pc_)) << 8U;
  const auto low = static_cast<std::uint16_t>(peek(static_cast<std::uint16_t>(pc_ + 1U)));
  return static_cast<std::uint16_t>(high | low);
}

void Emulator::step() {
  if (waiting_key_.has_value()) {
    return;
  }

  const std::uint16_t opcode = fetch_opcode();
  pc_ = static_cast<std::uint16_t>(pc_ + 2U);
  ++cycles_;
  execute(opcode);
}

void Emulator::tick_timers() {
  if (delay_timer_ > 0) {
    --delay_timer_;
  }
  if (sound_timer_ > 0) {
    --sound_timer_;
  }
}

void Emulator::execute(std::uint16_t opcode) {
  const auto x = static_cast<std::size_t>((opcode >> 8U) & 0x000FU);
  const auto y = static_cast<std::size_t>((opcode >> 4U) & 0x000FU);
  const auto n = static_cast<std::uint8_t>(opcode & 0x000FU);
  const auto nn = static_cast<std::uint8_t>(opcode & 0x00FFU);
  const auto nnn = static_cast<std::uint16_t>(opcode & 0x0FFFU);

  switch (opcode & 0xF000U) {
  case 0x0000:
    if (opcode == 0x00E0) {
      clear_display();
    } else if (opcode == 0x00EE) {
      pc_ = pop();
    } else {
      // 0NNN machine-code calls target the RCA 1802 and are not implemented.
      ++unsupported_opcodes_;
    }
    break;

  case 0x1000:
    pc_ = nnn;
    break;

  case 0x2000:
    push(pc_);
    pc_ = nnn;
    break;

  case 0x3000:
    if (v_[x] == nn) {
      pc_ = static_cast<std::uint16_t>(pc_ + 2U);
    }
    break;

  case 0x4000:
    if (v_[x] != nn) {
      pc_ = static_cast<std::uint16_t>(pc_ + 2U);
    }
    break;

  case 0x5000:
    if (n != 0) {
      ++unsupported_opcodes_;
    } else if (v_[x] == v_[y]) {
      pc_ = static_cast<std::uint16_t>(pc_ + 2U);
    }
    break;

  case 0x6000:
    v_[x] = nn;
    break;

  case 0x7000:
    v_[x] = static_cast<std::uint8_t>(v_[x] + nn);
    break;

  case 0x8000:
    switch (n) {
    case 0x0:
      v_[x] = v_[y];
      break;
    case 0x1:
      v_[x] = static_cast<std::uint8_t>(v_[x] | v_[y]);
      break;
    case 0x2:
      v_[x] = static_cast<std::uint8_t>(v_[x] & v_[y]);
      break;
    case 0x3:
      v_[x] = static_cast<std::uint8_t>(v_[x] ^ v_[y]);
      break;
    case 0x4: {
      const unsigned sum = static_cast<unsigned>(v_[x]) + static_cast<unsigned>(v_[y]);
      v_[0x0F] = sum > 0xFFU ? 1 : 0;
      v_[x] = static_cast<std::uint8_t>(sum & 0xFFU);
      break;
    }
    case 0x5: {
      const std::uint8_t no_borrow = v_[x] >= v_[y] ? 1 : 0;
      v_[x] = static_cast<std::uint8_t>(v_[x] - v_[y]);
      v_[0x0F] = no_borrow;
      break;
    }
    case 0x6: {
      // Original CHIP-8: shifts VX in place and ignores VY.
      v_[0x0F] = static_cast<std::uint8_t>(v_[x] & 0x01U);
      v_[x] = static_cast<std::uint8_t>(v_[x] >> 1U);
      break;
    }
    case 0x7: {
      const std::uint8_t no_borrow = v_[y] >= v_[x] ? 1 : 0;
      v_[x] = static_cast<std::uint8_t>(v_[y] - v_[x]);
      v_[0x0F] = no_borrow;
      break;
    }
    case 0xE: {
      v_[0x0F] = static_cast<std::uint8_t>((v_[x] >> 7U) & 0x01U);
      v_[x] = static_cast<std::uint8_t>(v_[x] << 1U);
      break;
    }
    default:
      ++unsupported_opcodes_;
      break;
    }
    break;

  case 0x9000:
    if (n != 0) {
      ++unsupported_opcodes_;
    } else if (v_[x] != v_[y]) {
      pc_ = static_cast<std::uint16_t>(pc_ + 2U);
    }
    break;

  case 0xA000:
    i_ = nnn;
    break;

  case 0xB000:
    pc_ = static_cast<std::uint16_t>(nnn + v_[0]);
    break;

  case 0xC000:
    v_[x] = static_cast<std::uint8_t>(random_byte() & nn);
    break;

  case 0xD000:
    draw_sprite(v_[x], v_[y], n);
    break;

  case 0xE000:
    if (nn == 0x9E) {
      if (keys_[v_[x] & 0x0FU]) {
        pc_ = static_cast<std::uint16_t>(pc_ + 2U);
      }
    } else if (nn == 0xA1) {
      if (!keys_[v_[x] & 0x0FU]) {
        pc_ = static_cast<std::uint16_t>(pc_ + 2U);
      }
    } else {
      ++unsupported_opcodes_;
    }
    break;

  case 0xF000:
    switch (nn) {
    case 0x07:
      v_[x] = delay_timer_;
      break;
    case 0x0A:
      waiting_key_ = static_cast<std::uint8_t>(x);
      break;
    case 0x15:
      delay_timer_ = v_[x];
      break;
    case 0x18:
      sound_timer_ = v_[x];
      break;
    case 0x1E:
      i_ = static_cast<std::uint16_t>(i_ + v_[x]);
      break;
    case 0x29:
      i_ = static_cast<std::uint16_t>(kFontStart + (v_[x] & 0x0FU) * kFontGlyphBytes);
      break;
    case 0x33: {
      const std::uint8_t value = v_[x];
      memory_[i_] = static_cast<std::uint8_t>(value / 100U);
      memory_[static_cast<std::size_t>(i_) + 1U] = static_cast<std::uint8_t>((value / 10U) % 10U);
      memory_[static_cast<std::size_t>(i_) + 2U] = static_cast<std::uint8_t>(value % 10U);
      break;
    }
    case 0x55:
      // Original CHIP-8 leaves I unchanged.
      for (std::size_t index = 0; index <= x; ++index) {
        memory_[static_cast<std::size_t>(i_) + index] = v_[index];
      }
      break;
    case 0x65:
      for (std::size_t index = 0; index <= x; ++index) {
        v_[index] = memory_[static_cast<std::size_t>(i_) + index];
      }
      break;
    default:
      ++unsupported_opcodes_;
      break;
    }
    break;

  default:
    ++unsupported_opcodes_;
    break;
  }
}

std::uint8_t Emulator::v(std::size_t index) const {
  assert(index < kRegisterCount);
  return v_[index];
}

bool Emulator::pixel(int x, int y) const {
  if (x < 0 || y < 0 || x >= kDisplayWidth || y >= kDisplayHeight) {
    return false;
  }
  const auto index = static_cast<std::size_t>(y) * static_cast<std::size_t>(kDisplayWidth) +
                     static_cast<std::size_t>(x);
  return display_[index] != 0;
}

void Emulator::clear_display() {
  display_.fill(0);
}

void Emulator::set_key(std::uint8_t key, bool down) {
  if (static_cast<std::size_t>(key) >= kKeyCount) {
    return;
  }
  keys_[key] = down;

  if (down && waiting_key_.has_value()) {
    v_[*waiting_key_] = key;
    waiting_key_.reset();
  }
}

bool Emulator::key_down(std::uint8_t key) const {
  if (static_cast<std::size_t>(key) >= kKeyCount) {
    return false;
  }
  return keys_[key];
}

void Emulator::push(std::uint16_t value) {
  if (static_cast<std::size_t>(sp_) >= kStackDepth) {
    ++unsupported_opcodes_;
    return;
  }
  stack_[sp_] = value;
  ++sp_;
}

std::uint16_t Emulator::pop() {
  if (sp_ == 0) {
    ++unsupported_opcodes_;
    return 0;
  }
  --sp_;
  return stack_[sp_];
}

void Emulator::draw_sprite(std::uint8_t origin_x, std::uint8_t origin_y, std::uint8_t rows) {
  const auto start_x = static_cast<int>(origin_x % static_cast<std::uint8_t>(kDisplayWidth));
  const auto start_y = static_cast<int>(origin_y % static_cast<std::uint8_t>(kDisplayHeight));
  v_[0x0F] = 0;

  for (std::uint8_t row = 0; row < rows; ++row) {
    const std::uint8_t bits = memory_[static_cast<std::size_t>(i_) + static_cast<std::size_t>(row)];
    const int py = (start_y + static_cast<int>(row)) % kDisplayHeight;

    for (int column = 0; column < 8; ++column) {
      if ((bits & (0x80U >> column)) == 0) {
        continue;
      }
      const int px = (start_x + column) % kDisplayWidth;
      auto& target =
          display_[static_cast<std::size_t>(py) * static_cast<std::size_t>(kDisplayWidth) +
                   static_cast<std::size_t>(px)];
      if (target != 0) {
        v_[0x0F] = 1;
      }
      target = static_cast<std::uint8_t>(target ^ 0x01U);
    }
  }
}

std::uint8_t Emulator::random_byte() {
  return static_cast<std::uint8_t>(rng_() & 0xFFU);
}

} // namespace chip8
