#include "pager.hpp"
#include <cstring>
#include <utility>

Pager::Pager(std::fstream&& file, std::filesystem::path filename) 

: file(std::move(file)), filename(std::move(filename))  

{
    
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
    return Pager(std::move(file), filename);
}

std::uint64_t Pager::pageCount() const {
    std::uintmax_t file_size = std::filesystem::file_size(filename);

    std::uint64_t count = static_cast<std::uint64_t> (file_size) / PAGE_SIZE;

    if(count == 0) return 0; // uint64_t wraps (bcz it's unsigned). so -1 becomes 18,446,744,073,709,551,615 making every page number valid

    return count - 1;
}

std::expected<void, PagerError> Pager::readPage(std::uint64_t page_number, std::array<char, PAGE_SIZE>& buffer) {
    if(!pageExists(page_number)) return std::unexpected(PagerError::INVALID_PAGE);

    file.clear(); // clearing any previous operation failed state

    file.seekg(pageOffset(page_number), std::ios::beg);

    if(file.fail()) return std::unexpected(PagerError::READ_FAILED);

    file.read(buffer.data(), buffer.size());

    if(file.fail()) return std::unexpected(PagerError::READ_FAILED);

    return {};
}

std::expected<std::uint64_t, PagerError> Pager::allocatePage() {
    std::array<char, PAGE_SIZE> page = {};

    std::uint64_t page_number = pageCount();

    file.clear();
    file.seekp(0, std::ios::end); // Putting the pointer at the end

    if(file.fail()) return std::unexpected(PagerError::PAGE_ALLOCATE_FAILED);

    file.write(page.data(), page.size());

    if(file.fail()) return std::unexpected(PagerError::PAGE_ALLOCATE_FAILED);

    file.flush();

    if(file.fail()) return std::unexpected(PagerError::PAGE_ALLOCATE_FAILED);


    return page_number;
}

std::expected<void, PagerError> Pager::writePage(std::uint64_t page_number, const std::array<char, PAGE_SIZE> &buffer) {
    if(!pageExists(page_number)) return std::unexpected(PagerError::INVALID_PAGE);

    file.clear();
    file.seekp(pageOffset(page_number), std::ios::beg);

    if(file.fail()) return std::unexpected(PagerError::WRITE_FAILED);

    file.write(buffer.data(), buffer.size());

    if(file.fail()) return std::unexpected(PagerError::WRITE_FAILED);

    file.flush();

    if(file.fail()) return std::unexpected(PagerError::WRITE_FAILED);

    return {};
}

//Helpers

bool Pager::pageExists(std::uint64_t page_number) const {    
    return page_number < pageCount();
}

std::streamoff Pager::pageOffset(const std::uint64_t page_number) const {
    return static_cast<std::streamoff>(page_number + 1) * static_cast<std::streamoff> (PAGE_SIZE);
}