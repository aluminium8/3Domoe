#include <MITSUDomoe/GuiClient.hpp>
#include <MITSUDomoe/Logger.hpp>
#include <memory>

int main(int, char **)
{
    auto log_path = MITSU_Domoe::initialize_logger();
    auto client = std::make_unique<MITSU_Domoe::GuiClient>(log_path);
    client->run();
    return 0;
}
