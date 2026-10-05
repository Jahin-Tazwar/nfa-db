#include "pager.hpp"
#include <filesystem>
#include <array>
#include <cstring>
#include <utility>

Pager::Pager(std::fstream&& file) : file(std::move(file))  {
    
}

std::expected<void, PagerError> Pager::create(const char* filename) {
    if(std::filesystem::exists(filename)) return std::unexpected(PagerError::FILE_ALREADY_EXISTS);

    std::fstream file(filename, std::ios::in | std::ios::out | std::ios::binary | std::ios::trunc);

    if(!file.is_open()) return std::unexpected(PagerError::FILE_CREATE_FAILED);

    std::array<char, PAGE_SIZE> page = {};

    std::memcpy(page.data(), MAGIC, MAGIC_SIZE);

    std::memcpy(
        page.data() + MAGIC_SIZE, 
        reinterpret_cast<const char*> (&VERSION), 
        sizeof(VERSION)
    ); // adding the sizes for offset. It will determine where to start writing from

    std::memcpy(
        page.data() + MAGIC_SIZE + sizeof(VERSION), 
        reinterpret_cast<const char*> (&PAGE_SIZE), 
        sizeof(PAGE_SIZE)
    );

    // Writes the data and reserve the rest
    file.write(page.data(), page.size());

    if(file.fail()) return std::unexpected(PagerError::WRITE_FAILED);

    file.close();

    if(file.fail()) return std::unexpected(PagerError::WRITE_FAILED);

    // For success
    return {};
}

std::expected<Pager, PagerError> Pager::open(const char* filename) {
    std::fstream file(filename, std::ios::binary | std::ios::in | std::ios::out);

    // Open check
    if(!file.is_open()) return std::unexpected(PagerError::FILE_OPEN_FAILED);

    file.seekg(0, std::ios::end);
    std::streamoff file_size = file.tellg();
    
    // Size check
    if(file_size < 0) return std::unexpected(PagerError::FILE_CORRUPTED);
    
    // File smaller than 1 page check
    if(file_size < static_cast<std::streamoff> (PAGE_SIZE)) return std::unexpected(PagerError::FILE_CORRUPTED);

    // Complete pages check
    if(file_size % static_cast<std::streamoff> (PAGE_SIZE) != 0) return std::unexpected(PagerError::FILE_CORRUPTED);

    // Moving the pointer to the begining
    file.seekg(0, std::ios::beg); // seekg(0, position) -> move 0 bytes from the position. 0 is the offset

    char magic[MAGIC_SIZE];
    std::uint32_t version;
    std::uint32_t page_size;

    file.read(magic, MAGIC_SIZE);
    file.read(reinterpret_cast<char*> (&version), sizeof(version));
    file.read(reinterpret_cast<char*> (&page_size), sizeof(page_size));

    if(file.fail()) return std::unexpected(PagerError::FILE_CORRUPTED);

    //Check magic number
    if(std::memcmp(magic, MAGIC, MAGIC_SIZE) != 0) return std::unexpected(PagerError::FILE_CORRUPTED);

    //Check version
    if(version != VERSION) return std::unexpected(PagerError::VERSION_MISMATCH);
    
    // Check page_size
    if(page_size != PAGE_SIZE) return std::unexpected(PagerError::FILE_CORRUPTED);

    // Construct Pager
    return Pager(std::move(file));
}