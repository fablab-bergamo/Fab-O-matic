#ifndef LCDWRAPPER_HPP_
#define LCDWRAPPER_HPP_

#include "BoardInfo.hpp"
#include "Machine.hpp"
#include "pins.hpp"
#include <array>
#include <chrono>
#include <LiquidCrystal.h>

namespace fabomatic
{
  /// @brief LCD display wrapper
  class LCDWrapper
  {
  public:
    LCDWrapper(const pins_config::lcd_config &config);

    using DisplayBuffer = std::array<std::array<char, conf::lcd::COLS>, conf::lcd::ROWS>;

    auto begin() -> bool;
    auto clear() -> void;
    auto showConnection(bool show) -> void;
    auto showPower(bool show) -> void;
    auto setRow(uint8_t row, const std::string_view &text) -> void;
    auto update(const BoardInfo &boardinfo, bool forced = false) -> void;

    [[nodiscard]] auto convertSecondsToHHMMSS(std::chrono::seconds duration) const -> const std::string;

  private:
    static constexpr auto HEIGHT_PX = 8;
    static constexpr auto CHAR_ANTENNA = 0;
    static constexpr auto CHAR_CONNECTION = 1;
    static constexpr auto CHAR_NO_CONNECTION = 2;
    static constexpr auto CHAR_POWERED_ON = 3;
    static constexpr auto CHAR_POWERED_OFF = 4;
    static constexpr auto CHAR_POWERING_OFF = 5;
    // Character definitions
    static constexpr std::array<uint8_t, HEIGHT_PX> antenna_char{0x15, 0x0E, 0x04, 0x04, 0x04, 0x04, 0x04, 0x04};
    static constexpr std::array<uint8_t, HEIGHT_PX> connection_char{0x00, 0x00, 0x01, 0x01, 0x05, 0x05, 0x15, 0x15};
    static constexpr std::array<uint8_t, HEIGHT_PX> noconnection_char{0x00, 0x00, 0x11, 0x0A, 0x04, 0x0A, 0x11, 0x00};
    static constexpr std::array<uint8_t, HEIGHT_PX> powered_on_char{0x04, 0x04, 0x04, 0x1f, 0x1f, 0x1f, 0x0a, 0x0a};
    static constexpr std::array<uint8_t, HEIGHT_PX> powered_off_char{0x0a, 0x04, 0x0a, 0x00, 0x1f, 0x1f, 0x0a, 0x0a};
    static constexpr std::array<uint8_t, HEIGHT_PX> powering_off_char{0x0e, 0x15, 0x15, 0x15, 0x17, 0x11, 0x11, 0x0e};

    const pins_config::lcd_config &config;

    LiquidCrystal lcd;
    bool show_connection_status;
    bool show_power_status;
    bool forceUpdate;
    DisplayBuffer buffer;
    DisplayBuffer current;
    BoardInfo boardInfo;

    auto backlightOn() const -> void;
    auto backlightOff() const -> void;
    auto prettyPrint(const DisplayBuffer &buffer, const BoardInfo &bi) const -> void;
    auto createChar(uint8_t char_idx, const std::array<uint8_t, HEIGHT_PX> &values) -> void;

    [[nodiscard]] auto needsUpdate(const BoardInfo &bi) const -> bool;
  };
} // namespace fabomatic

#endif // LCDWRAPPER_HPP_
