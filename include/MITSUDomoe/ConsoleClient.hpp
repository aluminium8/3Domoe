#pragma once

#include "BaseClient.hpp"

namespace MITSU_Domoe
{

#include <filesystem>

class ConsoleClient : public BaseClient
{
public:
    ConsoleClient(const std::filesystem::path& log_path);
    ~ConsoleClient() = default;

    void run() override;
};

}
