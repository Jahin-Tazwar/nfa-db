
#include "record.hpp"

#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <span>
#include <string>
#include <vector>

void check(bool condition, const std::string& message)
{
    if (!condition)
    {
        std::cerr << "FAILED: " << message << '\n';
        std::exit(EXIT_FAILURE);
    }
}

void check_error(RecordError actual, RecordError expected,
                 const std::string& message)
{
    check(actual == expected, message);
}

int main()
{
    const Schema schema = {
        ColumnType::INTEGER,
        ColumnType::TEXT,
        ColumnType::INTEGER
    };

    // 1. Basic serialization and exact byte format.
    {
        Row row = {7, "Alice", 30};

        auto result = serialize(schema, row);
        check(result.has_value(), "Basic serialization succeeds");

        const std::vector<std::byte> expected = {
            std::byte{0x07}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x00},

            std::byte{0x05}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x00},

            std::byte{0x41}, std::byte{0x6C},
            std::byte{0x69}, std::byte{0x63},
            std::byte{0x65},

            std::byte{0x1E}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x00}
        };

        check(*result == expected, "Exact serialized bytes match");

        auto decoded = deserialize(schema, *result);
        check(decoded.has_value(), "Basic deserialization succeeds");
        check(decoded->row == row, "Decoded row matches original");
        check(decoded->bytes_consumed == expected.size(),
              "Correct number of bytes consumed");
    }

    // 2. Empty text.
    {
        Row row = {0, "", 0};

        auto result = serialize(schema, row);
        check(result.has_value(), "Empty text serializes");

        auto decoded = deserialize(schema, *result);
        check(decoded.has_value(), "Empty text deserializes");
        check(decoded->row == row, "Empty text round-trips");
        check(decoded->bytes_consumed == 12,
              "Empty text record consumes 12 bytes");
    }

    // 3. Integer boundaries and negative values.
    {
        const std::vector<std::int32_t> values = {
            0, 1, -1, 127, 128, -128,
            255, 256, 32767, -32768,
            INT32_MAX, INT32_MIN
        };

        const Schema int_schema = {ColumnType::INTEGER};

        for (std::int32_t value : values)
        {
            Row row = {value};

            auto result = serialize(int_schema, row);
            check(result.has_value(), "Boundary integer serializes");

            auto decoded = deserialize(int_schema, *result);
            check(decoded.has_value(), "Boundary integer deserializes");
            check(decoded->row == row, "Boundary integer round-trips");
            check(decoded->bytes_consumed == 4,
                  "Integer consumes four bytes");
        }
    }

    // 4. Unicode text is preserved as UTF-8 bytes.
    {
        const Schema text_schema = {ColumnType::TEXT};
        Row row = {std::string("বাংলা 猫 é")};

        auto result = serialize(text_schema, row);
        check(result.has_value(), "Unicode text serializes");

        auto decoded = deserialize(text_schema, *result);
        check(decoded.has_value(), "Unicode text deserializes");
        check(decoded->row == row, "Unicode text round-trips");
    }

    // 5. Empty schema and row.
    {
        const Schema empty_schema = {};
        const Row empty_row = {};

        auto result = serialize(empty_schema, empty_row);
        check(result.has_value(), "Empty row serializes");
        check(result->empty(), "Empty row produces zero bytes");

        auto decoded = deserialize(empty_schema, *result);
        check(decoded.has_value(), "Empty row deserializes");
        check(decoded->row == empty_row, "Empty row round-trips");
        check(decoded->bytes_consumed == 0,
              "Empty row consumes zero bytes");
    }

    // 6. Wrong number of values.
    {
        Row row = {7, "Alice"};

        auto result = serialize(schema, row);
        check(!result.has_value(), "Wrong column count is rejected");
        check_error(result.error(), RecordError::SCHEMA_ROW_MISMATCH,
                    "Wrong column count returns schema mismatch");
    }

    // 7. Wrong value types.
    {
        Row row = {std::string("not an integer"), "Alice", 30};

        auto result = serialize(schema, row);
        check(!result.has_value(), "Wrong value type is rejected");
        check_error(result.error(), RecordError::SCHEMA_ROW_MISMATCH,
                    "Wrong value type returns schema mismatch");
    }

    // 8. Truncated integer.
    {
        const Schema int_schema = {ColumnType::INTEGER};

        const std::vector<std::byte> malformed = {
            std::byte{0x07}, std::byte{0x00}
        };

        auto result = deserialize(int_schema, malformed);
        check(!result.has_value(), "Truncated integer is rejected");
        check_error(result.error(), RecordError::INVALID_RECORD,
                    "Truncated integer returns invalid record");
    }

    // 9. Truncated text-length prefix.
    {
        const Schema text_schema = {ColumnType::TEXT};

        const std::vector<std::byte> malformed = {
            std::byte{0x05}, std::byte{0x00}
        };

        auto result = deserialize(text_schema, malformed);
        check(!result.has_value(), "Truncated text length is rejected");
        check_error(result.error(), RecordError::INVALID_RECORD,
                    "Truncated text length returns invalid record");
    }

    // 10. Text length exceeds the available bytes.
    {
        const Schema text_schema = {ColumnType::TEXT};

        const std::vector<std::byte> malformed = {
            std::byte{0x05}, std::byte{0x00},
            std::byte{0x00}, std::byte{0x00},
            std::byte{0x41}, std::byte{0x42}
        };

        auto result = deserialize(text_schema, malformed);
        check(!result.has_value(), "Truncated text is rejected");
        check_error(result.error(), RecordError::INVALID_RECORD,
                    "Truncated text returns invalid record");
    }

    // 11. Two records stored consecutively.
    {
        Row row1 = {7, "Alice", 30};
        Row row2 = {8, "Bob", 25};

        auto res1 = serialize(schema, row1);
        auto res2 = serialize(schema, row2);

        check(res1.has_value() && res2.has_value(),
              "Both records serialize");

        std::vector<std::byte> combined = *res1;
        combined.insert(combined.end(), res2->begin(), res2->end());

        std::span<const std::byte> all = combined;

        auto decoded1 = deserialize(schema, all);
        check(decoded1.has_value(), "First concatenated record decodes");
        check(decoded1->row == row1, "First record is correct");

        auto rest = all.subspan(decoded1->bytes_consumed);
        auto decoded2 = deserialize(schema, rest);

        check(decoded2.has_value(), "Second concatenated record decodes");
        check(decoded2->row == row2, "Second record is correct");

        check(decoded1->bytes_consumed + decoded2->bytes_consumed
                  == combined.size(),
              "Both records consume exactly the combined bytes");
    }

    std::cout << "All record tests passed!\n";
    return EXIT_SUCCESS;
}
