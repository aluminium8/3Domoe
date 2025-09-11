#pragma once

#include "BaseClient.hpp"
#include <vector>
#include <string>

namespace MITSU_Domoe
{

#include <filesystem>

class ConsoleClient : public BaseClient
{
public:
    ConsoleClient(const std::filesystem::path& log_path);
    ~ConsoleClient() = default;

    void run() override;

private:
    void print_help();
    void run_tests();
    void handle_load(const std::string& path_str);
    void handle_trace(const std::string& path_str);
};

}
