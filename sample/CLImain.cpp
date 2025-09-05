#include <MITSUDomoe/ConsoleClient.hpp>
#include <memory>

int main()
{
    auto client = std::make_unique<MITSU_Domoe::ConsoleClient>();
    client->run();
    return 0;
}
