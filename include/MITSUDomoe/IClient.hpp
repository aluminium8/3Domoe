#pragma once

#include <string>
#include <cstdint>
#include <optional>
#include <vector>
#include "ResultRepository.hpp"

namespace MITSU_Domoe
{

class IClient
{
public:
    virtual ~IClient() = default;
    virtual void run() = 0;

protected:
    virtual uint64_t post_command(const std::string& command_name, const std::string& json_input) = 0;
    virtual std::optional<CommandResult> get_result(uint64_t command_id) = 0;
    virtual std::map<uint64_t, CommandResult> get_all_results() = 0;
    virtual std::vector<std::string> get_command_names() = 0;
    virtual std::map<std::string, std::string> get_input_schema(const std::string& command_name) = 0;
};

}
