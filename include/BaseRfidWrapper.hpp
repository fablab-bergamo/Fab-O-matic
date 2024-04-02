#ifndef BASERFIDWRAPPER_HPP_
#define BASERFIDWRAPPER_HPP_
#include <optional>

#include "card.hpp"

namespace fablabbg
{
  class BaseRFIDWrapper
  {
  public:
    virtual ~BaseRFIDWrapper() = default;
    virtual auto rfidInit() const -> bool = 0;

    virtual auto isNewCardPresent() const -> bool = 0;
    virtual auto cardStillThere(const card::uid_t original,
                                std::chrono::milliseconds delay) const -> bool = 0;
    virtual auto getUid() const -> card::uid_t = 0;

    virtual auto readCardSerial() const -> std::optional<card::uid_t> = 0;
    virtual auto selfTest() const -> bool = 0;
    virtual auto reset() const -> void = 0;
  };
} // namespace fablabbg
#endif // BASERFIDWRAPPER_HPP_