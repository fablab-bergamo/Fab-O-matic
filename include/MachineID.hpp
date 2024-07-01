#ifndef MACHINE_ID_HPP
#define MACHINE_ID_HPP

#include <cstdint>

enum class MachineType : uint8_t
{
  Invalid = 0,
  Printer3D = 1,
  Laser = 2,
  Cnc = 3,
  PrinterResin = 4,
  Other = 5
};

/**
 * The unique machine identifier for the backend
 */
struct MachineID
{
  uint16_t id;

  constexpr MachineID() : id(0) {}
  constexpr MachineID(uint16_t id) : id(id) {}
  // Add conversion operator to uint16_t to use in print, streams...
  constexpr operator uint16_t() const { return id; }
};

#endif // MACHINE_ID_HPP