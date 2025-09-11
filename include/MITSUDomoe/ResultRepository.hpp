#pragma once

#include "ICartridge.hpp"
#include <map>
#include <mutex>
#include <optional>
#include <cstdint>

namespace MITSU_Domoe {

// (中身は前回と同じ)
class ResultRepository {
public:
    void store_result(uint64_t id, CommandResult result);
    std::optional<CommandResult> get_result(uint64_t id);
    bool remove_result(uint64_t id);
    std::map<uint64_t, CommandResult> get_all_results() const;
    std::optional<uint64_t> get_latest_result_id(uint64_t command_id_to_ignore) const;
    std::optional<uint64_t> get_nth_latest_result_id(size_t n, uint64_t command_id_to_ignore) const;

private:
    std::map<uint64_t, CommandResult> results_;
    mutable std::mutex mutex_;
};

} // namespace MITSU_Domoe
