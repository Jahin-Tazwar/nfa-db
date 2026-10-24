#include "record.hpp"
#include <limits>

static void write_uint32(std::vector<std::byte>& bytes, std::uint32_t value);
static std::uint32_t read_uint32(std::span<const std::byte> data, std::size_t offset);

std::expected<std::vector<std::byte>, RecordError> serialize(const Schema &schema, const Row &row) {
    std::vector<std::byte> bytes;

    if(row.size() != schema.size()) return std::unexpected(RecordError::SCHEMA_ROW_MISMATCH);

    for (std::size_t i = 0; i < schema.size(); i++)
    {
        if (schema[i] == ColumnType::INTEGER)
        {   
            const std::int32_t *num = std::get_if<std::int32_t> (&row[i]); // if std::int32_t is in row[i] then returns the pointer to it. Else nullptr

            if(num == nullptr) return std::unexpected(RecordError::SCHEMA_ROW_MISMATCH);

            write_uint32(bytes, static_cast<std::uint32_t> (*num));
        }
        else
        {   
           const std::string *text = std::get_if<std::string> (&row[i]);

           if(text == nullptr) return std::unexpected(RecordError::SCHEMA_ROW_MISMATCH);

            // Check if the text size is too big for int32_t
            if (text -> size() > static_cast<std::size_t>(std::numeric_limits<std::int32_t>::max())) {
                return std::unexpected(RecordError::STRING_TOO_LARGE);
            }

            std::uint32_t size = static_cast<std::uint32_t>(text -> size());

            // Appending the size of the text
            write_uint32(bytes, size);

            // Appending the text bytes
            for (char c : *text)
            {
                bytes.push_back(static_cast<std::byte>(static_cast<unsigned char>(c))); // unsigned so that it remains non-negative
            }
        }
    }

    return bytes;
}

std::expected<DeserializedRecord, RecordError> deserialize(const Schema &schema, std::span<const std::byte> data)
{
    Row row;

    std::size_t offset = 0;

    for (std::size_t i = 0; i < schema.size(); i++)
    {

        if (schema[i] == ColumnType::INTEGER)
        {

            if (data.size() - offset < 4)
                return std::unexpected(RecordError::INVALID_RECORD);

            // Reading the int
            std::uint32_t num = read_uint32(data, offset);

            offset += 4;

            Value num_val = static_cast<std::int32_t>(num);
            row.push_back(num_val);
        }
        else
        {
            // Reading the size
            if (data.size() - offset < 4)
                return std::unexpected(RecordError::INVALID_RECORD);
            
            std::uint32_t size = read_uint32(data, offset);

            offset += 4;

            // Reading the text
            std::string text;

            if (data.size() - offset < size)
                return std::unexpected(RecordError::INVALID_RECORD);

            for (std::uint32_t j = 0; j < size; j++)
            {
                text.push_back(
                    static_cast<char>(
                        std::to_integer<unsigned char>(data[offset + j])
                    )
                );
            }

            offset += static_cast<std::size_t>(size);

            row.push_back(text);
        }
    }

    return DeserializedRecord{std::move(row), offset};
}

// Helpers
static void write_uint32(std::vector<std::byte>& bytes, std::uint32_t value)
{
    for (std::size_t i = 0; i < 4; i++)
    {
        bytes.push_back(static_cast<std::byte>((value >> (8 * i)) & 0xFF)); // Gives the last 2 digits of hex value.  Shifts right 8 bit (dividing by 256) then gives the last 2 digits
    }
}

static std::uint32_t read_uint32(std::span<const std::byte> data, std::size_t offset)
{
    std::uint32_t value = 0;

    for (std::size_t i = 0; i < 4; i++)
    {
        std::uint32_t byte = std::to_integer<std::uint32_t>(data[offset + i]);
        value |= byte << (8 * i);
    }

    return value;
}