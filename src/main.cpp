#include <iostream>
#include "pager.hpp"

int main()
{
    auto create_result = Pager::create("nfa.bin");

    if (!create_result)
    {
        std::cout << "Create failed: "
                  << static_cast<int>(create_result.error())
                  << std::endl;

        return 1;
    }

    std::cout << "Create successful" << std::endl;

    auto open_result = Pager::open("missing.bin");

    if (!open_result)
    {
        std::cout << "Open failed: "
                  << static_cast<int>(open_result.error())
                  << std::endl;

        return 1;
    }

    std::cout << "Open successful" << std::endl;

    return 0;
}