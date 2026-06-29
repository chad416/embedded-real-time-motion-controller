#pragma once

#include <cstddef>
#include <map>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace emrtc {

class JsonValue {
public:
    using Array = std::vector<JsonValue>;
    using Object = std::map<std::string, JsonValue>;

    JsonValue();
    explicit JsonValue(bool value);
    explicit JsonValue(double value);
    explicit JsonValue(std::string value);
    explicit JsonValue(Array value);
    explicit JsonValue(Object value);

    bool is_null() const;
    bool is_bool() const;
    bool is_number() const;
    bool is_string() const;
    bool is_array() const;
    bool is_object() const;

    bool as_bool(const std::string& context) const;
    double as_number(const std::string& context) const;
    const std::string& as_string(const std::string& context) const;
    const Array& as_array(const std::string& context) const;
    const Object& as_object(const std::string& context) const;
    const JsonValue* find(const std::string& key) const;

private:
    using Storage = std::variant<std::nullptr_t, bool, double, std::string, Array, Object>;
    Storage value_;
};

JsonValue parse_json(std::string_view text, const std::string& source_name = "<memory>");
JsonValue load_json_file(const std::string& path);

}  // namespace emrtc
