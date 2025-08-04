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

private:
    std::map<uint64_t, CommandResult> results_;
    std::mutex mutex_;
};

} // namespace MITSU_Domoe
