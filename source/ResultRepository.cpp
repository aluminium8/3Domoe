#include <MITSUDomoe/ResultRepository.hpp>
#include <spdlog/spdlog.h>
#include <rfl/json.hpp>

namespace MITSU_Domoe
{

    // (中身は前回と同じ)
    void ResultRepository::store_result(uint64_t id, CommandResult result)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (const auto *success = std::get_if<SuccessResult>(&result))
        {
            const size_t max_display_length = 512;
            std::string result_str_for_display; // 表示用の文字列を準備

            // もし元の文字列が256文字より長ければ
            if ((success->output_json).size() > max_display_length)
            {
                // 表示用文字列を256文字で切り取り、末尾に "..." を追加

                result_str_for_display = success->output_json.substr(0,max_display_length) + "...";
                spdlog::debug("json trancated {} to {}", result_str_for_display.size(), success->output_json.size());
            }
            else
            {
                result_str_for_display = success->output_json;
            }

            spdlog::info("Storing successful result for command ID {}: {}", id, result_str_for_display);
        }
        else if (const auto *error = std::get_if<ErrorResult>(&result))
        {
            spdlog::error("Storing error result for command ID {}: {}", id, error->error_message);
        }
        results_[id] = std::move(result);
    }

    std::optional<CommandResult> ResultRepository::get_result(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        if (auto it = results_.find(id); it != results_.end())
        {
            return it->second;
        }
        return std::nullopt;
    }

    bool ResultRepository::remove_result(uint64_t id)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return results_.erase(id) > 0;
    }

    std::map<uint64_t, CommandResult> ResultRepository::get_all_results() const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        return results_;
    }

    std::optional<uint64_t> ResultRepository::get_latest_result_id(uint64_t command_id_to_ignore) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        for (auto it = results_.rbegin(); it != results_.rend(); ++it)
        {
            if (it->first < command_id_to_ignore)
            {
                return it->first;
            }
        }
        return std::nullopt;
    }

    std::optional<uint64_t> ResultRepository::get_nth_latest_result_id(size_t n, uint64_t command_id_to_ignore) const
    {
        std::lock_guard<std::mutex> lock(mutex_);
        std::vector<uint64_t> valid_ids;
        for (const auto &pair : results_)
        {
            if (pair.first < command_id_to_ignore)
            {
                valid_ids.push_back(pair.first);
            }
        }

        if (valid_ids.size() < n)
        {
            return std::nullopt;
        }

        return valid_ids[valid_ids.size() - n];
    }

} // namespace MITSU_Domoe
