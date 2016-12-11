#include <numeric>

#include "poll_utils.h"

namespace sst {
namespace util {
bool PollingData::check_waiting() {
    return std::accumulate(if_waiting.begin(), if_waiting.end(), false, [](bool a, bool b) {return a || b; });
}

void PollingData::insert_completion_entry(uint32_t index, std::pair<int32_t, int32_t> ce) {
    std::lock_guard<std::mutex> lk(poll_mutex);
    completion_entries[index].push_back(ce);
}

std::experimental::optional<std::pair<int32_t, int32_t>> PollingData::get_completion_entry(const std::thread::id id) {
    std::lock_guard<std::mutex> lk(poll_mutex);
    auto index = tid_to_index[id];
    if(completion_entries[index].empty()) {
        return {};
    }
    auto ce = completion_entries[index].front();
    completion_entries[index].pop_front();
    return ce;
}

uint32_t PollingData::get_index(const std::thread::id id) {
    std::lock_guard<std::mutex> lk(poll_mutex);
    if(tid_to_index.find(id) == tid_to_index.end()) {
        completion_entries.push_back(std::list<std::pair<int32_t, int32_t>>());
        tid_to_index[id] = completion_entries.size();
        if_waiting.push_back(false);
    }
    return tid_to_index[id];
}

void PollingData::set_waiting(const std::thread::id id) {
    std::lock_guard<std::mutex> lk(poll_mutex);
    auto index = tid_to_index[id];
    if_waiting[index] = true;
    poll_cv.notify_all();
}

void PollingData::reset_waiting(const std::thread::id id) {
    std::lock_guard<std::mutex> lk(poll_mutex);
    auto index = tid_to_index[id];
    if_waiting[index] = false;
}

void PollingData::wait_for_requests() {
    std::unique_lock<std::mutex> lk(poll_mutex);
    poll_cv.wait(lk, check_waiting);
}
}
}
