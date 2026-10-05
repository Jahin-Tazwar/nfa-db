#include <iostream>
#include <fstream>
#include <filesystem>
#include <cstdint>

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

            // Magic:   bytes 0-7
            // Version: bytes 8-11
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

            // Page size: bytes 12-15
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

    // 11. Exactly one valid header page
    {
        std::filesystem::remove(testFile);

        if (!createTestDatabase(testFile))
        {
            failures++;
        }
        else
        {
            auto result = Pager::open(testFile);

            bool passed = result.has_value();

            if (!printResult(
                    "exactly one valid header page",
                    passed))
            {
                failures++;
            }
        }

        std::filesystem::remove(testFile);
    }

    return failures == 0 ? 0 : 1;
}