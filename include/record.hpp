#pragma once

#include <variant>
#include <vector>
#include <cstdint>
#include <cstddef>
#include <span>
#include <string>
#include <expected>

enum class ColumnType {
    INTEGER,
    TEXT
};

enum class RecordError {
    INVALID_RECORD,
    SCHEMA_ROW_MISMATCH,
    STRING_TOO_LARGE
};



using Schema = std::vector<ColumnType>;

using Value = std::variant<std::int32_t, std::string>;

using Row = std::vector<Value>;

struct DeserializedRecord {
    Row row;
    std::size_t bytes_consumed;
};

std::expected<std::vector<std::byte>, RecordError> serialize(const Schema &schema, const Row &row);
std::expected<DeserializedRecord, RecordError> deserialize(const Schema &schema, std::span<const std::byte> data);

// std::span provides a non-owning view of bytes without copying them.
// Unlike a vector reference, it can represent a range of bytes from
// a vector, array, or database page.