#include "MITSUDomoe/BaseClient.hpp"

namespace MITSU_Domoe
{

BaseClient::BaseClient(const std::filesystem::path& log_path)
{
    result_repo = std::make_shared<ResultRepository>();
    processor = std::make_unique<CommandProcessor>(result_repo, log_path);
}

BaseClient::~BaseClient()
{
    if (processor) {
        processor->stop();
    }
}

uint64_t BaseClient::post_command(const std::string& command_name, const std::string& json_input)
{
    return processor->add_to_queue(command_name, json_input);
}

std::optional<CommandResult> BaseClient::get_result(uint64_t command_id)
{
    return result_repo->get_result(command_id);
}

std::map<uint64_t, CommandResult> BaseClient::get_all_results()
{
    return result_repo->get_all_results();
}

std::vector<std::string> BaseClient::get_command_names()
{
    return processor->get_command_names();
}

std::map<std::string, std::string> BaseClient::get_input_schema(const std::string& command_name)
{
    return processor->get_input_schema(command_name);
}

}
