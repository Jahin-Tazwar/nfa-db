#include <iostream>

#include "pager.hpp"

int main()
{
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

    Pager& pager = *pagerResult;

    std::cout << "Database opened successfully\n";
    std::cout << "Data pages: "
              << pager.pageCount()
              << std::endl;

    return 0;
}