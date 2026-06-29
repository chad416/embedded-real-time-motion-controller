#include "emrtc/json.hpp"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace emrtc {
namespace {

class JsonParser {
public:
    JsonParser(std::string_view text, std::string source_name)
        : text_(text), source_name_(std::move(source_name)) {}

    JsonValue parse() {
        skip_whitespace();
        auto value = parse_value();
        skip_whitespace();
        if (position_ != text_.size()) {
            fail("unexpected trailing content");
        }
        return value;
    }

private:
    [[noreturn]] void fail(const std::string& message) const {
        std::size_t line = 1;
        std::size_t column = 1;
        for (std::size_t index = 0; index < position_ && index < text_.size(); ++index) {
            if (text_[index] == '\n') {
                ++line;
                column = 1;
            } else {
                ++column;
            }
        }
        std::ostringstream detail;
        detail << source_name_ << ':' << line << ':' << column << ": " << message;
        throw std::runtime_error(detail.str());
    }

    void skip_whitespace() {
        while (position_ < text_.size()) {
            const char value = text_[position_];
            if (value != ' ' && value != '\t' && value != '\r' && value != '\n') {
                break;
            }
            ++position_;
        }
    }

    bool consume(char expected) {
        if (position_ < text_.size() && text_[position_] == expected) {
            ++position_;
            return true;
        }
        return false;
    }

    void expect(char expected, const std::string& message) {
        if (!consume(expected)) {
            fail(message);
        }
    }

    JsonValue parse_value() {
        if (position_ >= text_.size()) {
            fail("expected a JSON value");
        }
        switch (text_[position_]) {
            case 'n': parse_literal("null"); return JsonValue();
            case 't': parse_literal("true"); return JsonValue(true);
            case 'f': parse_literal("false"); return JsonValue(false);
            case '"': return JsonValue(parse_string());
            case '[': return JsonValue(parse_array());
            case '{': return JsonValue(parse_object());
            default:
                if (text_[position_] == '-' ||
                    (text_[position_] >= '0' && text_[position_] <= '9')) {
                    return JsonValue(parse_number());
                }
                fail("expected null, boolean, number, string, array, or object");
        }
    }

    void parse_literal(std::string_view literal) {
        if (text_.substr(position_, literal.size()) != literal) {
            fail("invalid JSON literal");
        }
        position_ += literal.size();
    }

    static void append_utf8(std::string& output, std::uint32_t code_point) {
        if (code_point <= 0x7FU) {
            output.push_back(static_cast<char>(code_point));
        } else if (code_point <= 0x7FFU) {
            output.push_back(static_cast<char>(0xC0U | (code_point >> 6U)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        } else if (code_point <= 0xFFFFU) {
            output.push_back(static_cast<char>(0xE0U | (code_point >> 12U)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        } else {
            output.push_back(static_cast<char>(0xF0U | (code_point >> 18U)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 12U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | ((code_point >> 6U) & 0x3FU)));
            output.push_back(static_cast<char>(0x80U | (code_point & 0x3FU)));
        }
    }

    std::uint32_t parse_hex_quad() {
        if (text_.size() - position_ < 4U) {
            fail("incomplete Unicode escape");
        }
        std::uint32_t value = 0;
        for (int index = 0; index < 4; ++index) {
            const char digit = text_[position_++];
            value <<= 4U;
            if (digit >= '0' && digit <= '9') value |= static_cast<std::uint32_t>(digit - '0');
            else if (digit >= 'a' && digit <= 'f') value |= static_cast<std::uint32_t>(digit - 'a' + 10);
            else if (digit >= 'A' && digit <= 'F') value |= static_cast<std::uint32_t>(digit - 'A' + 10);
            else fail("invalid hexadecimal digit in Unicode escape");
        }
        return value;
    }

    std::string parse_string() {
        expect('"', "expected string");
        std::string output;
        while (position_ < text_.size()) {
            const unsigned char value = static_cast<unsigned char>(text_[position_++]);
            if (value == '"') {
                return output;
            }
            if (value < 0x20U) {
                fail("unescaped control character in string");
            }
            if (value != '\\') {
                output.push_back(static_cast<char>(value));
                continue;
            }
            if (position_ >= text_.size()) {
                fail("incomplete string escape");
            }
            const char escape = text_[position_++];
            switch (escape) {
                case '"': output.push_back('"'); break;
                case '\\': output.push_back('\\'); break;
                case '/': output.push_back('/'); break;
                case 'b': output.push_back('\b'); break;
                case 'f': output.push_back('\f'); break;
                case 'n': output.push_back('\n'); break;
                case 'r': output.push_back('\r'); break;
                case 't': output.push_back('\t'); break;
                case 'u': {
                    std::uint32_t code_point = parse_hex_quad();
                    if (code_point >= 0xD800U && code_point <= 0xDBFFU) {
                        if (text_.size() - position_ < 2U ||
                            text_[position_] != '\\' || text_[position_ + 1] != 'u') {
                            fail("high surrogate must be followed by a low surrogate");
                        }
                        position_ += 2;
                        const std::uint32_t low = parse_hex_quad();
                        if (low < 0xDC00U || low > 0xDFFFU) {
                            fail("invalid low surrogate");
                        }
                        code_point = 0x10000U + ((code_point - 0xD800U) << 10U) + (low - 0xDC00U);
                    } else if (code_point >= 0xDC00U && code_point <= 0xDFFFU) {
                        fail("unexpected low surrogate");
                    }
                    append_utf8(output, code_point);
                    break;
                }
                default: fail("invalid string escape");
            }
        }
        fail("unterminated string");
    }

    double parse_number() {
        const std::size_t start = position_;
        consume('-');
        if (consume('0')) {
            if (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
                fail("leading zero is not allowed in a JSON number");
            }
        } else {
            if (position_ >= text_.size() || text_[position_] < '1' || text_[position_] > '9') {
                fail("expected digit in JSON number");
            }
            while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
                ++position_;
            }
        }
        if (consume('.')) {
            if (position_ >= text_.size() || text_[position_] < '0' || text_[position_] > '9') {
                fail("fraction requires at least one digit");
            }
            while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
                ++position_;
            }
        }
        if (position_ < text_.size() && (text_[position_] == 'e' || text_[position_] == 'E')) {
            ++position_;
            if (position_ < text_.size() && (text_[position_] == '+' || text_[position_] == '-')) {
                ++position_;
            }
            if (position_ >= text_.size() || text_[position_] < '0' || text_[position_] > '9') {
                fail("exponent requires at least one digit");
            }
            while (position_ < text_.size() && text_[position_] >= '0' && text_[position_] <= '9') {
                ++position_;
            }
        }
        const std::string token(text_.substr(start, position_ - start));
        try {
            const double value = std::stod(token);
            if (!std::isfinite(value)) {
                fail("JSON number is outside the finite double range");
            }
            return value;
        } catch (const std::exception&) {
            fail("invalid JSON number");
        }
    }

    JsonValue::Array parse_array() {
        expect('[', "expected array");
        skip_whitespace();
        JsonValue::Array values;
        if (consume(']')) return values;
        while (true) {
            skip_whitespace();
            values.push_back(parse_value());
            skip_whitespace();
            if (consume(']')) return values;
            expect(',', "expected ',' or ']' in array");
            skip_whitespace();
            if (position_ < text_.size() && text_[position_] == ']') {
                fail("trailing comma is not allowed in array");
            }
        }
    }

    JsonValue::Object parse_object() {
        expect('{', "expected object");
        skip_whitespace();
        JsonValue::Object values;
        if (consume('}')) return values;
        while (true) {
            skip_whitespace();
            if (position_ >= text_.size() || text_[position_] != '"') {
                fail("object key must be a string");
            }
            const auto key = parse_string();
            skip_whitespace();
            expect(':', "expected ':' after object key");
            skip_whitespace();
            auto inserted = values.emplace(key, parse_value());
            if (!inserted.second) {
                fail("duplicate object key: " + key);
            }
            skip_whitespace();
            if (consume('}')) return values;
            expect(',', "expected ',' or '}' in object");
            skip_whitespace();
            if (position_ < text_.size() && text_[position_] == '}') {
                fail("trailing comma is not allowed in object");
            }
        }
    }

    std::string_view text_;
    std::string source_name_;
    std::size_t position_{0};
};

[[noreturn]] void wrong_type(const std::string& context, const char* expected) {
    throw std::runtime_error(context + " must be " + expected);
}

}  // namespace

JsonValue::JsonValue() : value_(nullptr) {}
JsonValue::JsonValue(bool value) : value_(value) {}
JsonValue::JsonValue(double value) : value_(value) {}
JsonValue::JsonValue(std::string value) : value_(std::move(value)) {}
JsonValue::JsonValue(Array value) : value_(std::move(value)) {}
JsonValue::JsonValue(Object value) : value_(std::move(value)) {}

bool JsonValue::is_null() const { return std::holds_alternative<std::nullptr_t>(value_); }
bool JsonValue::is_bool() const { return std::holds_alternative<bool>(value_); }
bool JsonValue::is_number() const { return std::holds_alternative<double>(value_); }
bool JsonValue::is_string() const { return std::holds_alternative<std::string>(value_); }
bool JsonValue::is_array() const { return std::holds_alternative<Array>(value_); }
bool JsonValue::is_object() const { return std::holds_alternative<Object>(value_); }

bool JsonValue::as_bool(const std::string& context) const {
    if (!is_bool()) wrong_type(context, "a boolean");
    return std::get<bool>(value_);
}

double JsonValue::as_number(const std::string& context) const {
    if (!is_number()) wrong_type(context, "a number");
    return std::get<double>(value_);
}

const std::string& JsonValue::as_string(const std::string& context) const {
    if (!is_string()) wrong_type(context, "a string");
    return std::get<std::string>(value_);
}

const JsonValue::Array& JsonValue::as_array(const std::string& context) const {
    if (!is_array()) wrong_type(context, "an array");
    return std::get<Array>(value_);
}

const JsonValue::Object& JsonValue::as_object(const std::string& context) const {
    if (!is_object()) wrong_type(context, "an object");
    return std::get<Object>(value_);
}

const JsonValue* JsonValue::find(const std::string& key) const {
    if (!is_object()) return nullptr;
    const auto& object = std::get<Object>(value_);
    const auto item = object.find(key);
    return item == object.end() ? nullptr : &item->second;
}

JsonValue parse_json(std::string_view text, const std::string& source_name) {
    return JsonParser(text, source_name).parse();
}

JsonValue load_json_file(const std::string& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("cannot open JSON file: " + path);
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parse_json(buffer.str(), path);
}

}  // namespace emrtc
