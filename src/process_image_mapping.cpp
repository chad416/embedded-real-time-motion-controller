#include "emrtc/process_image_mapping.hpp"

#include <cmath>
#include <limits>

namespace emrtc {

const std::array<Cia402ObjectMapping, 6>& cia402_process_image_mapping() {
    static const std::array<Cia402ObjectMapping, 6> mapping{{
        {"controlword", 0x6040, 0x00, 16, true, "CiA 402 controlword"},
        {"statusword", 0x6041, 0x00, 16, false, "CiA 402 statusword"},
        {"target_position", 0x607A, 0x00, 32, true, "Target position"},
        {"target_velocity", 0x60FF, 0x00, 32, true, "Target velocity"},
        {"actual_position", 0x6064, 0x00, 32, false, "Position actual value"},
        {"actual_velocity", 0x606C, 0x00, 32, false, "Velocity actual value"},
    }};
    return mapping;
}

const Cia402ObjectMapping* find_cia402_mapping(const std::string& field) {
    const auto& mapping = cia402_process_image_mapping();
    for (const auto& item : mapping) {
        if (field == item.field) {
            return &item;
        }
    }
    return nullptr;
}

double normalized_counts_per_unit(double counts_per_unit) {
    return counts_per_unit > 0.0 ? counts_per_unit : 1.0;
}

std::int32_t units_to_counts(double units, double counts_per_unit) {
    const double scale = normalized_counts_per_unit(counts_per_unit);
    const double scaled = units * scale;
    const double max_value = static_cast<double>(std::numeric_limits<std::int32_t>::max());
    const double min_value = static_cast<double>(std::numeric_limits<std::int32_t>::min());
    if (scaled >= max_value) {
        return std::numeric_limits<std::int32_t>::max();
    }
    if (scaled <= min_value) {
        return std::numeric_limits<std::int32_t>::min();
    }
    return static_cast<std::int32_t>(std::lround(scaled));
}

double counts_to_units(std::int32_t counts, double counts_per_unit) {
    return static_cast<double>(counts) / normalized_counts_per_unit(counts_per_unit);
}

Cia402WireImage encode_cia402_wire_image(const ProcessImage& image, double counts_per_unit) {
    Cia402WireImage wire_image;
    wire_image.controlword = image.controlword;
    wire_image.statusword = image.statusword;
    wire_image.target_position_counts = units_to_counts(image.target_position, counts_per_unit);
    wire_image.target_velocity_counts = units_to_counts(image.target_velocity, counts_per_unit);
    wire_image.actual_position_counts = units_to_counts(image.actual_position, counts_per_unit);
    wire_image.actual_velocity_counts = units_to_counts(image.actual_velocity, counts_per_unit);
    wire_image.drive_fault = image.drive_fault;
    return wire_image;
}

ProcessImage decode_cia402_wire_image(const Cia402WireImage& wire_image, double counts_per_unit) {
    ProcessImage image;
    image.controlword = wire_image.controlword;
    image.statusword = wire_image.statusword;
    image.target_position = counts_to_units(wire_image.target_position_counts, counts_per_unit);
    image.target_velocity = counts_to_units(wire_image.target_velocity_counts, counts_per_unit);
    image.actual_position = counts_to_units(wire_image.actual_position_counts, counts_per_unit);
    image.actual_velocity = counts_to_units(wire_image.actual_velocity_counts, counts_per_unit);
    image.drive_fault = wire_image.drive_fault;
    return image;
}

}  // namespace emrtc
