#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>
#include <array>

#include "pager.hpp"

bool printResult(const char* testName, bool passed)
{
    std::cout << (passed ? "[PASS] " : "[FAIL] ")
              << testName << '\n';

    return passed;
}

bool createTestDatabase(const char* filename)
{
    auto result = Pager::create(filename);

    if (!result)
    {
        std::cout << "Test setup failed: create() returned error "
                  << static_cast<int>(result.error())
                  << '\n';

        return false;
    }

    return true;
}

int main()
{
    const char* testFile = "nfa_test.bin";

    int failures = 0;

    // 1. Open missing file
    {
        std::filesystem::remove(testFile);

        auto result = Pager::open(testFile);

        bool passed =
            !result &&
            result.error() == PagerError::FILE_OPEN_FAILED;

        if (!printResult("open missing file", passed))
            failures++;
    }

    // 2. Create then open
    {
        std::filesystem::remove(testFile);

        auto createResult = Pager::create(testFile);

        bool passed = false;

        if (createResult)
        {
            auto openResult = Pager::open(testFile);
            passed = openResult.has_value();
        }

        if (!printResult("create then open", passed))
            failures++;

        std::filesystem::remove(testFile);
    }

    // 3. Create twice
    {
        std::filesystem::remove(testFile);

        auto first = Pager::create(testFile);
        auto second = Pager::create(testFile);

        bool passed =
            first &&
            !second &&
            second.error() == PagerError::FILE_ALREADY_EXISTS;

        if (!printResult("create twice", passed))
            failures++;

        std::filesystem::remove(testFile);
    }

    // 4. 100 bytes of junk
    {
        std::filesystem::remove(testFile);

        std::ofstream file(
            testFile,
            std::ios::binary | std::ios::trunc
        );

        const char junk[100] = {};

        file.write(junk, sizeof(junk));
        file.close();

        auto result = Pager::open(testFile);

        bool passed =
            !result &&
            result.error() == PagerError::FILE_CORRUPTED;

        if (!printResult("100 bytes of junk", passed))
            failures++;

        std::filesystem::remove(testFile);
    }

    // 5. 4096 bytes of zeros
    {
        std::filesystem::remove(testFile);

        std::ofstream file(
            testFile,
            std::ios::binary | std::ios::trunc
        );

        const char zeros[Pager::PAGE_SIZE] = {};

        file.write(zeros, sizeof(zeros));
        file.close();

        auto result = Pager::open(testFile);

        bool passed =
            !result &&
            result.error() == PagerError::FILE_CORRUPTED;

        if (!printResult("4096 bytes of zeros", passed))
            failures++;

        std::filesystem::remove(testFile);
    }

    // 6. Valid header but 5000 bytes total
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            std::ofstream file(
                testFile,
                std::ios::binary | std::ios::app
            );

            const char extra[904] = {};

            file.write(extra, sizeof(extra));
            file.close();

            auto result = Pager::open(testFile);

            bool passed =
                !result &&
                result.error() == PagerError::FILE_CORRUPTED;

            if (!printResult(
                    "valid header + 5000 bytes",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 7. Valid header but version = 2
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            std::fstream file(
                testFile,
                std::ios::binary |
                std::ios::in |
                std::ios::out
            );

            std::uint32_t badVersion = 2;

            file.seekp(8, std::ios::beg);

            file.write(
                reinterpret_cast<const char*>(&badVersion),
                sizeof(badVersion)
            );

            file.close();

            auto result = Pager::open(testFile);

            bool passed =
                !result &&
                result.error() == PagerError::VERSION_MISMATCH;

            if (!printResult(
                    "valid header + version 2",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 8. Valid header but wrong page size
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            std::fstream file(
                testFile,
                std::ios::binary |
                std::ios::in |
                std::ios::out
            );

            std::uint32_t badPageSize = 8192;

            file.seekp(12, std::ios::beg);

            file.write(
                reinterpret_cast<const char*>(&badPageSize),
                sizeof(badPageSize)
            );

            file.close();

            auto result = Pager::open(testFile);

            bool passed =
                !result &&
                result.error() == PagerError::FILE_CORRUPTED;

            if (!printResult(
                    "valid header + wrong page size",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 9. Valid header but wrong magic
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            std::fstream file(
                testFile,
                std::ios::binary |
                std::ios::in |
                std::ios::out
            );

            const char badMagic[8] = {
                'B', 'A', 'D', '-', 'D', 'B', '\0', '\0'
            };

            file.seekp(0, std::ios::beg);

            file.write(
                badMagic,
                sizeof(badMagic)
            );

            file.close();

            auto result = Pager::open(testFile);

            bool passed =
                !result &&
                result.error() == PagerError::FILE_CORRUPTED;

            if (!printResult(
                    "valid header + wrong magic",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 10. 0-byte file
    {
        std::filesystem::remove(testFile);

        std::ofstream file(
            testFile,
            std::ios::binary | std::ios::trunc
        );

        file.close();

        auto result = Pager::open(testFile);

        bool passed =
            !result &&
            result.error() == PagerError::FILE_CORRUPTED;

        if (!printResult("0-byte file", passed))
            failures++;

        std::filesystem::remove(testFile);
    }

    // 11. Header only means zero data pages
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed =
                result &&
                result->pageCount() == 0;

            if (!printResult(
                    "header only has zero data pages",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 12. Allocate first data page
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                auto allocateResult =
                    result->allocatePage();

                passed =
                    allocateResult &&
                    allocateResult.value() == 0 &&
                    result->pageCount() == 1;
            }

            if (!printResult(
                    "allocate first data page",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 13. Allocate multiple data pages
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                auto first = result->allocatePage();
                auto second = result->allocatePage();

                passed =
                    first &&
                    second &&
                    first.value() == 0 &&
                    second.value() == 1 &&
                    result->pageCount() == 2;
            }

            if (!printResult(
                    "allocate multiple data pages",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 14. Truncated file does not wrap page count
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                std::filesystem::resize_file(
                    testFile,
                    100
                );

                passed =
                    result->pageCount() == 0;
            }

            if (!printResult(
                    "truncated file does not wrap page count",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 15. Cannot read a data page when none exists
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                std::array<char, Pager::PAGE_SIZE> buffer{};

                auto readResult =
                    result->readPage(0, buffer);

                passed =
                    !readResult &&
                    readResult.error() == PagerError::INVALID_PAGE;
            }

            if (!printResult(
                    "read page when no data pages exist",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 16. Read first data page
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                auto allocateResult =
                    result->allocatePage();

                if (allocateResult)
                {
                    std::array<char, Pager::PAGE_SIZE> buffer{};

                    auto readResult =
                        result->readPage(0, buffer);

                    passed = readResult.has_value();
                }
            }

            if (!printResult(
                    "read first data page",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 17. Write and read first data page
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                auto allocateResult =
                    result->allocatePage();

                if (allocateResult)
                {
                    std::array<char, Pager::PAGE_SIZE> writeBuffer{};
                    writeBuffer.fill('A');

                    auto writeResult =
                        result->writePage(0, writeBuffer);

                    if (writeResult)
                    {
                        std::array<char, Pager::PAGE_SIZE> readBuffer{};

                        auto readResult =
                            result->readPage(0, readBuffer);

                        passed =
                            readResult &&
                            readBuffer[0] == 'A' &&
                            readBuffer[Pager::PAGE_SIZE - 1] == 'A';
                    }
                }
            }

            if (!printResult(
                    "write and read first data page",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 18. Cannot read a non-existent data page
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                auto allocateResult =
                    result->allocatePage();

                if (allocateResult)
                {
                    std::array<char, Pager::PAGE_SIZE> buffer{};

                    auto readResult =
                        result->readPage(1, buffer);

                    passed =
                        !readResult &&
                        readResult.error() == PagerError::INVALID_PAGE;
                }
            }

            if (!printResult(
                    "read non-existent data page",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 19. Write and read multiple data pages
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                auto first = result->allocatePage();
                auto second = result->allocatePage();

                if (first && second)
                {
                    std::array<char, Pager::PAGE_SIZE> pageA{};
                    std::array<char, Pager::PAGE_SIZE> pageB{};
                    std::array<char, Pager::PAGE_SIZE> readA{};
                    std::array<char, Pager::PAGE_SIZE> readB{};

                    pageA.fill('A');
                    pageB.fill('B');

                    auto writeA =
                        result->writePage(0, pageA);

                    auto writeB =
                        result->writePage(1, pageB);

                    auto readResultA =
                        result->readPage(0, readA);

                    auto readResultB =
                        result->readPage(1, readB);

                    passed =
                        writeA &&
                        writeB &&
                        readResultA &&
                        readResultB &&
                        readA[0] == 'A' &&
                        readA[Pager::PAGE_SIZE - 1] == 'A' &&
                        readB[0] == 'B' &&
                        readB[Pager::PAGE_SIZE - 1] == 'B';
                }
            }

            if (!printResult(
                    "write and read multiple data pages",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    // 20. Cannot write to a non-existent data page
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = false;

            if (result)
            {
                std::array<char, Pager::PAGE_SIZE> buffer{};
                buffer.fill('X');

                auto writeResult =
                    result->writePage(0, buffer);

                passed =
                    !writeResult &&
                    writeResult.error() == PagerError::INVALID_PAGE;
            }

            if (!printResult(
                    "write non-existent data page",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    std::cout << '\n';

    if (failures == 0)
    {
        std::cout << "All tests passed.\n";
        return 0;
    }

    std::cout << failures
              << " test(s) failed.\n";

    return 1;
}