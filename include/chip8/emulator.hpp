#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

namespace chip8 {

inline constexpr std::size_t kMemorySize = 4096;
inline constexpr std::uint16_t kProgramStart = 0x200;
inline constexpr std::uint16_t kFontStart = 0x50;
inline constexpr int kDisplayWidth = 64;
inline constexpr int kDisplayHeight = 32;
inline constexpr std::size_t kRegisterCount = 16;
inline constexpr std::size_t kStackDepth = 16;
inline constexpr std::size_t kKeyCount = 16;

/// Capacity available to a ROM, in bytes.
inline constexpr std::size_t kMaxRomSize = kMemorySize - kProgramStart;

enum class LoadError : std::uint8_t {
  none,
  empty,
  too_large,
};

/// Headless CHIP-8 interpreter.
///
/// The core is deliberately free of I/O: the front end owns the window, the
/// 60 Hz clock, and the audio. `step()` executes exactly one instruction and
/// does not allocate.
class Emulator {
public:
  Emulator();

  /// Clears memory, registers, display, and timers, then reloads the font set.
  /// The program counter returns to kProgramStart.
  void reset();

  /// Copies a ROM into memory at kProgramStart and resets the machine first.
  /// The pointer must reference at least `size` bytes.
  LoadError load_rom(const std::uint8_t* data, std::size_t size);
  LoadError load_rom(const std::vector<std::uint8_t>& rom) {
    return load_rom(rom.data(), rom.size());
  }

  /// Fetches, decodes, and executes one instruction. A no-op while waiting for
  /// a key press.
  void step();

  /// Decrements the delay and sound timers by one tick. Call at 60 Hz.
  void tick_timers();

  std::uint16_t pc() const { return pc_; }
  std::uint16_t i_register() const { return i_; }
  std::uint8_t v(std::size_t index) const;
  std::uint8_t delay_timer() const { return delay_timer_; }
  std::uint8_t sound_timer() const { return sound_timer_; }
  std::uint64_t cycles() const { return cycles_; }

  /// Number of opcodes seen that this interpreter does not implement.
  std::uint32_t unsupported_opcodes() const { return unsupported_opcodes_; }

  bool waiting_for_key() const { return waiting_key_.has_value(); }

  /// Bounds-checked framebuffer read; false for out-of-range coordinates.
  bool pixel(int x, int y) const;
  void clear_display();

  /// Read-only view of address space, for tests and debuggers.
  std::uint8_t peek(std::uint16_t address) const;

  /// Sets a key state. While waiting for a key (FX0A), the first key pressed
  /// is stored in VX and releases the wait.
  void set_key(std::uint8_t key, bool down);
  bool key_down(std::uint8_t key) const;

private:
  std::uint16_t fetch_opcode() const;
  void execute(std::uint16_t opcode);

  void push(std::uint16_t value);
  std::uint16_t pop();
  void draw_sprite(std::uint8_t origin_x, std::uint8_t origin_y, std::uint8_t rows);
  void load_font();
  std::uint8_t random_byte();

  std::array<std::uint8_t, kMemorySize> memory_{};
  std::array<std::uint8_t, kRegisterCount> v_{};
  std::array<std::uint16_t, kStackDepth> stack_{};
  std::array<std::uint8_t,
             static_cast<std::size_t>(kDisplayWidth) * static_cast<std::size_t>(kDisplayHeight)>
      display_{};
  std::array<bool, kKeyCount> keys_{};

  std::uint16_t i_ = 0;
  std::uint16_t pc_ = kProgramStart;
  std::uint8_t sp_ = 0;
  std::uint8_t delay_timer_ = 0;
  std::uint8_t sound_timer_ = 0;
  std::optional<std::uint8_t> waiting_key_;
  std::mt19937 rng_{0x5EEDU};
  std::uint64_t cycles_ = 0;
  std::uint32_t unsupported_opcodes_ = 0;
};

} // namespace chip8
