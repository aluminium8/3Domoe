#include <MITSUDomoe/GuiClient.hpp>
#include <memory>

int main(int, char**)
{
    auto client = std::make_unique<MITSU_Domoe::GuiClient>();
    client->run();
    return 0;
}
