#include <iostream>
#include <fstream>
#include <cstdint>

int main()
{
    std::int32_t out = 1;
    std::int32_t out2 = 256;
    {
        std::ofstream f("demo.bin", std::ios::binary);
        f.write(reinterpret_cast<char *>(&out), sizeof(out));
        f.write(reinterpret_cast<char *>(&out2), sizeof(out2));
    }

    std::int32_t in = 0;
    std::int32_t in2 = 0;

    std::ifstream g("demo.bin", std::ios::binary);

    if (!g.is_open())
    {
        std::cout << "Could not open file\n";
        return 1;
    }

    g.read(reinterpret_cast<char *>(&in), sizeof(in));

    if (g.fail())
    {
        std::cout << "Read 1 failed\n";
        return 1;
    }

    g.read(reinterpret_cast<char *>(&in2), sizeof(in2));

    if (g.fail())
    {
        std::cout << "Read 2 failed\n";
        return 1;
    }

    std::cout << in << '\n';
    std::cout << in2 << '\n';
    return 0;
}