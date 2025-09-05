#pragma once

#include "IClient.hpp"
#include "CommandProcessor.hpp"
#include "ResultRepository.hpp"
#include <memory>

namespace MITSU_Domoe
{

class BaseClient : public IClient
{
public:
    BaseClient();
    virtual ~BaseClient();

    void run() override = 0;

protected:
    uint64_t post_command(const std::string& command_name, const std::string& json_input) override;
    std::optional<CommandResult> get_result(uint64_t command_id) override;
    std::map<uint64_t, CommandResult> get_all_results() override;
    std::vector<std::string> get_command_names() override;
    std::map<std::string, std::string> get_input_schema(const std::string& command_name) override;

    std::shared_ptr<ResultRepository> result_repo;
    std::unique_ptr<CommandProcessor> processor;
};

}
