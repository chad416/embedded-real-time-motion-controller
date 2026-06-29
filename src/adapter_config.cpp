#include "emrtc/adapter_config.hpp"

#include "emrtc/json.hpp"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <limits>
#include <set>
#include <sstream>
#include <stdexcept>

namespace emrtc {
namespace {

const JsonValue& required(const JsonValue::Object& object,
                          const char* key,
                          const std::string& context) {
    const auto item = object.find(key);
    if (item == object.end()) {
        throw std::runtime_error(context + " is missing required field '" + std::string(key) + "'");
    }
    return item->second;
}

void reject_unknown(const JsonValue::Object& object,
                    const std::set<std::string>& allowed,
                    const std::string& context) {
    for (const auto& item : object) {
        if (allowed.count(item.first) == 0U) {
            throw std::runtime_error(context + " contains unknown field '" + item.first + "'");
        }
    }
}

std::uint64_t unsigned_integer(const JsonValue& value,
                               const std::string& context,
                               std::uint64_t maximum = std::numeric_limits<std::uint64_t>::max()) {
    const double number = value.as_number(context);
    if (number < 0.0 || std::floor(number) != number ||
        number > static_cast<double>(maximum)) {
        throw std::runtime_error(context + " must be an unsigned integer in range");
    }
    return static_cast<std::uint64_t>(number);
}

std::uint32_t object_index(const JsonValue& value, const std::string& context) {
    if (value.is_number()) {
        return static_cast<std::uint32_t>(unsigned_integer(value, context, 0xFFFFU));
    }
    const auto& text = value.as_string(context);
    std::size_t consumed = 0;
    unsigned long parsed = 0;
    try {
        parsed = std::stoul(text, &consumed, 0);
    } catch (const std::exception&) {
        throw std::runtime_error(context + " must be a 16-bit integer or 0x-prefixed string");
    }
    if (consumed != text.size() || parsed > 0xFFFFUL) {
        throw std::runtime_error(context + " must be a 16-bit integer or 0x-prefixed string");
    }
    return static_cast<std::uint32_t>(parsed);
}

std::vector<AdapterObjectMapping> parse_mapping(const JsonValue& value,
                                                const std::string& context) {
    std::vector<AdapterObjectMapping> mappings;
    const auto& array = value.as_array(context);
    for (std::size_t index = 0; index < array.size(); ++index) {
        const std::string item_context = context + '[' + std::to_string(index) + ']';
        const auto& object = array[index].as_object(item_context);
        reject_unknown(object,
                       {"field", "object_index", "sub_index", "bit_length", "controller_to_drive"},
                       item_context);
        AdapterObjectMapping mapping;
        mapping.field = required(object, "field", item_context).as_string(item_context + ".field");
        mapping.object_index = static_cast<std::uint16_t>(
            object_index(required(object, "object_index", item_context), item_context + ".object_index"));
        mapping.sub_index = static_cast<std::uint8_t>(unsigned_integer(
            required(object, "sub_index", item_context), item_context + ".sub_index", 0xFFU));
        mapping.bit_length = static_cast<std::uint8_t>(unsigned_integer(
            required(object, "bit_length", item_context), item_context + ".bit_length", 64U));
        mapping.controller_to_drive =
            required(object, "controller_to_drive", item_context)
                .as_bool(item_context + ".controller_to_drive");
        mappings.push_back(std::move(mapping));
    }
    return mappings;
}

SoemPdoLayout parse_soem_layout(const JsonValue& value, const std::string& context) {
    const auto& object = value.as_object(context);
    reject_unknown(object,
                   {"controlword_offset", "target_position_offset", "target_velocity_offset",
                    "statusword_offset", "actual_position_offset", "actual_velocity_offset"},
                   context);
    SoemPdoLayout layout;
    layout.controlword_offset = static_cast<std::size_t>(unsigned_integer(
        required(object, "controlword_offset", context), context + ".controlword_offset"));
    layout.target_position_offset = static_cast<std::size_t>(unsigned_integer(
        required(object, "target_position_offset", context), context + ".target_position_offset"));
    layout.target_velocity_offset = static_cast<std::size_t>(unsigned_integer(
        required(object, "target_velocity_offset", context), context + ".target_velocity_offset"));
    layout.statusword_offset = static_cast<std::size_t>(unsigned_integer(
        required(object, "statusword_offset", context), context + ".statusword_offset"));
    layout.actual_position_offset = static_cast<std::size_t>(unsigned_integer(
        required(object, "actual_position_offset", context), context + ".actual_position_offset"));
    layout.actual_velocity_offset = static_cast<std::size_t>(unsigned_integer(
        required(object, "actual_velocity_offset", context), context + ".actual_velocity_offset"));
    return layout;
}

bool range_fits(std::size_t offset, std::size_t width, std::size_t available) {
    return offset <= available && width <= available - offset;
}

void validate_mapping(const std::vector<AdapterObjectMapping>& mapping) {
    const auto result = validate_adapter_mapping(mapping);
    if (!result.ok) {
        throw std::runtime_error("object_mapping: " + result.detail);
    }
}

}  // namespace

RuntimeAdapterConfig load_runtime_adapter_config(const std::string& path) {
    const auto root = load_json_file(path);
    const auto& object = root.as_object(path);

    RuntimeAdapterConfig config;
    config.schema_version = static_cast<std::uint32_t>(unsigned_integer(
        required(object, "schema_version", path), path + ".schema_version", 0xFFFFFFFFU));
    const std::string backend_context = path + ".backend";
    const std::string interface_context = path + ".interface_name";
    const std::string backend = required(object, "backend", path).as_string(backend_context);
    const std::string interface_name =
        required(object, "interface_name", path).as_string(interface_context);

    if (backend == "soem_ethercat") {
        reject_unknown(object,
                       {"schema_version", "backend", "interface_name", "group",
                        "io_map_bytes", "expected_slave_count", "receive_timeout_us",
                        "counts_per_unit", "pdo_layout", "object_mapping"},
                       path);
        config.backend = RuntimeAdapterBackend::SoemEtherCat;
        auto& soem = config.soem;
        soem.interface_name = interface_name;
        soem.group = static_cast<std::uint8_t>(unsigned_integer(
            required(object, "group", path), path + ".group", 0xFFU));
        soem.io_map_bytes = static_cast<std::size_t>(unsigned_integer(
            required(object, "io_map_bytes", path), path + ".io_map_bytes"));
        soem.expected_slave_count = static_cast<std::size_t>(unsigned_integer(
            required(object, "expected_slave_count", path), path + ".expected_slave_count"));
        soem.receive_timeout_us = static_cast<std::uint32_t>(unsigned_integer(
            required(object, "receive_timeout_us", path), path + ".receive_timeout_us", 0xFFFFFFFFU));
        soem.counts_per_unit = required(object, "counts_per_unit", path).as_number(path + ".counts_per_unit");
        soem.layout = parse_soem_layout(required(object, "pdo_layout", path), path + ".pdo_layout");
        soem.observed_mapping = parse_mapping(required(object, "object_mapping", path), path + ".object_mapping");
    } else if (backend == "socketcan_canopen") {
        reject_unknown(object,
                       {"schema_version", "backend", "interface_name", "node_id",
                        "receive_timeout_us", "heartbeat_timeout_cycles",
                        "counts_per_unit", "require_tpdo_each_cycle", "cob_ids",
                        "object_mapping"},
                       path);
        config.backend = RuntimeAdapterBackend::SocketCanCanOpen;
        auto& canopen = config.socketcan;
        canopen.interface_name = interface_name;
        canopen.node_id = static_cast<std::uint8_t>(unsigned_integer(
            required(object, "node_id", path), path + ".node_id", 0xFFU));
        canopen.receive_timeout_us = static_cast<std::uint32_t>(unsigned_integer(
            required(object, "receive_timeout_us", path), path + ".receive_timeout_us", 0xFFFFFFFFU));
        canopen.heartbeat_timeout_cycles = static_cast<std::uint32_t>(unsigned_integer(
            required(object, "heartbeat_timeout_cycles", path),
            path + ".heartbeat_timeout_cycles", 0xFFFFFFFFU));
        canopen.counts_per_unit = required(object, "counts_per_unit", path).as_number(path + ".counts_per_unit");
        canopen.require_tpdo_each_cycle =
            required(object, "require_tpdo_each_cycle", path)
                .as_bool(path + ".require_tpdo_each_cycle");
        const std::string ids_context = path + ".cob_ids";
        const JsonValue& ids_value = required(object, "cob_ids", path);
        const auto& ids = ids_value.as_object(ids_context);
        reject_unknown(ids, {"rpdo1", "rpdo2", "tpdo1", "tpdo2", "heartbeat"}, ids_context);
        canopen.rpdo1_cob_id = static_cast<std::uint32_t>(unsigned_integer(
            required(ids, "rpdo1", path + ".cob_ids"), path + ".cob_ids.rpdo1", 0x7FFU));
        canopen.rpdo2_cob_id = static_cast<std::uint32_t>(unsigned_integer(
            required(ids, "rpdo2", path + ".cob_ids"), path + ".cob_ids.rpdo2", 0x7FFU));
        canopen.tpdo1_cob_id = static_cast<std::uint32_t>(unsigned_integer(
            required(ids, "tpdo1", path + ".cob_ids"), path + ".cob_ids.tpdo1", 0x7FFU));
        canopen.tpdo2_cob_id = static_cast<std::uint32_t>(unsigned_integer(
            required(ids, "tpdo2", path + ".cob_ids"), path + ".cob_ids.tpdo2", 0x7FFU));
        canopen.heartbeat_cob_id = static_cast<std::uint32_t>(unsigned_integer(
            required(ids, "heartbeat", path + ".cob_ids"), path + ".cob_ids.heartbeat", 0x7FFU));
        canopen.observed_mapping =
            parse_mapping(required(object, "object_mapping", path), path + ".object_mapping");
    } else {
        throw std::runtime_error(path + ".backend must be 'soem_ethercat' or 'socketcan_canopen'");
    }

    validate_runtime_adapter_config(config);
    return config;
}

void validate_runtime_adapter_config(const RuntimeAdapterConfig& config) {
    if (config.schema_version != 1U) {
        throw std::runtime_error("schema_version must be 1");
    }
    if (config.backend == RuntimeAdapterBackend::SoemEtherCat) {
        const auto& soem = config.soem;
        if (soem.interface_name.empty()) throw std::runtime_error("interface_name must not be empty");
        if (soem.io_map_bytes < 4096U) throw std::runtime_error("io_map_bytes must be at least 4096");
        if (soem.expected_slave_count != 1U) throw std::runtime_error("expected_slave_count must be 1");
        if (soem.receive_timeout_us == 0U ||
            soem.receive_timeout_us > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
            throw std::runtime_error("receive_timeout_us must fit a positive int");
        }
        if (!std::isfinite(soem.counts_per_unit) || soem.counts_per_unit <= 0.0) {
            throw std::runtime_error("counts_per_unit must be finite and positive");
        }
        if (!range_fits(soem.layout.controlword_offset, 2U, soem.io_map_bytes) ||
            !range_fits(soem.layout.target_position_offset, 4U, soem.io_map_bytes) ||
            !range_fits(soem.layout.target_velocity_offset, 4U, soem.io_map_bytes) ||
            !range_fits(soem.layout.statusword_offset, 2U, soem.io_map_bytes) ||
            !range_fits(soem.layout.actual_position_offset, 4U, soem.io_map_bytes) ||
            !range_fits(soem.layout.actual_velocity_offset, 4U, soem.io_map_bytes)) {
            throw std::runtime_error("pdo_layout exceeds io_map_bytes");
        }
        validate_mapping(soem.observed_mapping);
        return;
    }

    const auto& canopen = config.socketcan;
    if (canopen.interface_name.empty()) throw std::runtime_error("interface_name must not be empty");
    if (canopen.node_id == 0U || canopen.node_id > 127U) {
        throw std::runtime_error("node_id must be in the CANopen range 1..127");
    }
    if (canopen.receive_timeout_us == 0U ||
        canopen.receive_timeout_us > static_cast<std::uint32_t>(std::numeric_limits<int>::max())) {
        throw std::runtime_error("receive_timeout_us must fit a positive int");
    }
    if (canopen.heartbeat_timeout_cycles == 0U) {
        throw std::runtime_error("heartbeat_timeout_cycles must be positive");
    }
    if (!std::isfinite(canopen.counts_per_unit) || canopen.counts_per_unit <= 0.0) {
        throw std::runtime_error("counts_per_unit must be finite and positive");
    }
    const std::set<std::uint32_t> ids{
        canopen.rpdo1_cob_id, canopen.rpdo2_cob_id, canopen.tpdo1_cob_id,
        canopen.tpdo2_cob_id, canopen.heartbeat_cob_id};
    if (ids.size() != 5U || ids.count(0U) != 0U || *ids.rbegin() > 0x7FFU) {
        throw std::runtime_error("cob_ids must be unique nonzero 11-bit CAN identifiers");
    }
    validate_mapping(canopen.observed_mapping);
}

std::string summarize_runtime_adapter_config(const RuntimeAdapterConfig& config) {
    std::ostringstream out;
    out << "schema_version=" << config.schema_version << '\n';
    out << "backend=" << to_string(config.backend) << '\n';
    if (config.backend == RuntimeAdapterBackend::SoemEtherCat) {
        const auto& soem = config.soem;
        out << "interface_name=" << soem.interface_name << '\n';
        out << "group=" << static_cast<unsigned int>(soem.group) << '\n';
        out << "io_map_bytes=" << soem.io_map_bytes << '\n';
        out << "expected_slave_count=" << soem.expected_slave_count << '\n';
        out << "receive_timeout_us=" << soem.receive_timeout_us << '\n';
        out << "counts_per_unit=" << std::setprecision(12) << soem.counts_per_unit << '\n';
        out << "mapping_entries=" << soem.observed_mapping.size() << '\n';
    } else {
        const auto& canopen = config.socketcan;
        out << "interface_name=" << canopen.interface_name << '\n';
        out << "node_id=" << static_cast<unsigned int>(canopen.node_id) << '\n';
        out << "receive_timeout_us=" << canopen.receive_timeout_us << '\n';
        out << "heartbeat_timeout_cycles=" << canopen.heartbeat_timeout_cycles << '\n';
        out << "counts_per_unit=" << std::setprecision(12) << canopen.counts_per_unit << '\n';
        out << "require_tpdo_each_cycle=" << (canopen.require_tpdo_each_cycle ? "true" : "false") << '\n';
        out << "mapping_entries=" << canopen.observed_mapping.size() << '\n';
    }
    out << "validation=PASS\n";
    out << "hardware_access=NONE\n";
    return out.str();
}

std::string to_string(RuntimeAdapterBackend backend) {
    switch (backend) {
        case RuntimeAdapterBackend::SoemEtherCat: return "soem_ethercat";
        case RuntimeAdapterBackend::SocketCanCanOpen: return "socketcan_canopen";
    }
    return "unknown";
}

}  // namespace emrtc
