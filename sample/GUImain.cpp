#include <MITSUDomoe/GuiClient.hpp>
#include <memory>
#include <spdlog/spdlog.h>
int main(int, char **)
{
    spdlog::set_level(spdlog::level::debug); // ★この行を追加
    auto client = std::make_unique<MITSU_Domoe::GuiClient>();
    client->run();
    return 0;
}
