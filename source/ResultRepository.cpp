#include <MITSUDomoe/ResultRepository.hpp>

namespace MITSU_Domoe {

// (中身は前回と同じ)
void ResultRepository::store_result(uint64_t id, CommandResult result) {
    std::lock_guard<std::mutex> lock(mutex_);
    results_[id] = std::move(result);
}

std::optional<CommandResult> ResultRepository::get_result(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (auto it = results_.find(id); it != results_.end()) {
        return it->second;
    }
    return std::nullopt;
}

bool ResultRepository::remove_result(uint64_t id) {
    std::lock_guard<std::mutex> lock(mutex_);
    return results_.erase(id) > 0;
}

} // namespace MITSU_Domoe
