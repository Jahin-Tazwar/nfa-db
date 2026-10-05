#pragma once

#include <array>
#include <cstdint>
#include <fstream>
#include <expected>

enum class PagerError {
    INVALID_PAGE,
    READ_FAILED,
    WRITE_FAILED,

    FILE_OPEN_FAILED,
    FILE_CREATE_FAILED,

    FILE_CORRUPTED,
    VERSION_MISMATCH,

    FILE_ALREADY_EXISTS
};


class Pager
{
private:
    static constexpr std::size_t MAGIC_SIZE = 8;

    static constexpr char MAGIC[MAGIC_SIZE] = {
        'N', 'F', 'A', '-', 'D', 'B', '\0', '\0'
    };

    static constexpr std::uint32_t VERSION = 1;
    
    std::fstream file;
    
    bool pageExists(std::uint64_t page_number) const;

    //Constructor
    explicit Pager(std::fstream&& file);

public:
    static constexpr std::uint32_t PAGE_SIZE = 4096;

    static std::expected<void, PagerError> create(const char* filename);

    static std::expected<Pager, PagerError> open(const char* filename);

    std::expected<void, PagerError> readPage(std::uint64_t page_number,
                  std::array<char, PAGE_SIZE>& buffer);

    std::expected<void, PagerError> writePage(std::uint64_t page_number,
                   const std::array<char, PAGE_SIZE>& buffer);

    std::expected<std::uint64_t, PagerError> allocatePage();

    std::uint64_t pageCount() const;

};