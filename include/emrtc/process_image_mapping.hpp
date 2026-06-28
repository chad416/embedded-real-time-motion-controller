#pragma once

#include "emrtc/bus.hpp"

#include <array>
#include <cstdint>
#include <string>

namespace emrtc {

struct Cia402ObjectMapping {
    const char* field;
    std::uint16_t object_index;
    std::uint8_t sub_index;
    std::uint8_t bit_length;
    bool controller_to_drive;
    const char* description;
};

struct Cia402WireImage {
    std::uint16_t controlword{0};
    std::uint16_t statusword{0};
    std::int32_t target_position_counts{0};
    std::int32_t target_velocity_counts{0};
    std::int32_t actual_position_counts{0};
    std::int32_t actual_velocity_counts{0};
    FaultCode drive_fault{FaultCode::None};
};

const std::array<Cia402ObjectMapping, 6>& cia402_process_image_mapping();
const Cia402ObjectMapping* find_cia402_mapping(const std::string& field);

std::int32_t units_to_counts(double units, double counts_per_unit);
double counts_to_units(std::int32_t counts, double counts_per_unit);

Cia402WireImage encode_cia402_wire_image(const ProcessImage& image, double counts_per_unit);
ProcessImage decode_cia402_wire_image(const Cia402WireImage& wire_image, double counts_per_unit);

}  // namespace emrtc
