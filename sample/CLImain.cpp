#include <MITSUDomoe/ConsoleClient.hpp>
#include <MITSUDomoe/Logger.hpp>
#include <memory>

int main()
{
    auto log_path = MITSU_Domoe::initialize_logger();
    auto client = std::make_unique<MITSU_Domoe::ConsoleClient>(log_path);
    client->run();
    return 0;
}
