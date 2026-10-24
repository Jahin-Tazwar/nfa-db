#include <iostream>
#include <iomanip>

#include "pager.hpp"
#include "record.hpp"

int main()
{

    Schema schema_g = {
        ColumnType::INTEGER,
        ColumnType::TEXT,
        ColumnType::INTEGER};

    Row row_g = {
        7,
        "Alice",
        30};

    std::expected<std::vector<std::byte>, RecordError> bytes = serialize(schema_g, row_g);

   if (!bytes) {
        std::cout << "Serialization failed!\n";
        return 1;
    }

    for (std::byte b : bytes.value())
    {
        std::cout << std::hex
                  << std::setw(2) << std::setfill('0')
                  << std::to_integer<int>(b) << " ";
    }
    std::cout << std::endl;

    std::expected<DeserializedRecord, RecordError> des_row = deserialize(schema_g, bytes.value());

    if (des_row.has_value()) {
        std::cout << "Deserialization successful!\n";

        Row result = des_row.value().row;
        std::size_t consumed_bytes = des_row.value().bytes_consumed;

        std::cout << std::dec;

        std::cout << std::get<std::int32_t>(result[0]) << '\n';
        std::cout << std::get<std::string>(result[1]) << '\n';
        std::cout << std::get<std::int32_t>(result[2]) << '\n';
        std::cout << "Consumed Bytes: " << consumed_bytes << std::endl;
    } else {
        std::cout << "Deserialization failed!\n";
    }

    std::cout << std::endl;

    auto createResult = Pager::create("nfa.bin");

    if (!createResult)
    {
        std::cout << "Create failed: "
                  << static_cast<int>(createResult.error())
                  << std::endl;

        return 1;
    }

    auto pagerResult = Pager::open("nfa.bin");

    if (!pagerResult)
    {
        std::cout << "Open failed: "
                  << static_cast<int>(pagerResult.error())
                  << std::endl;

        return 1;
    }

    Pager &pager = *pagerResult;

    std::cout << "Database opened successfully\n";
    std::cout << "Data pages: "
              << pager.pageCount()
              << std::endl;

    return 0;
}