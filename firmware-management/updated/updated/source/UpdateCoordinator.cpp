#include "UpdateCoordinator.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace updated {

void UpdateCoordinator::start(const std::filesystem::path &payload_path, const std::string header_data)
{
    assert(!payload_path.empty());
    assert(header_data.size());

    std::unique_lock<std::mutex> ul{ mutex };
    update_manifest.header = header_data;
    std::filesystem::create_directories(update_payload_path);
    update_payload_path /= payload_path.filename();
    std::filesystem::create_hard_link(payload_path, update_payload_path);
    //TODO: set global update state to UPDATING
    ul.unlock();
    condition_var.notify_all();
}

void UpdateCoordinator::run()
{
    std::unique_lock<std::mutex> ul{ mutex };
    // TODO: query UpdateTracker state to guard against spurious wakeups
    condition_var.wait(ul, [this](){ return update_manifest.header.size() > 0; });
    // TODO: call swupdate
    std::cout << "call swupdate" << '\n';
    ul.unlock();
    condition_var.notify_all();
}

Manifest UpdateCoordinator::get_manifest() const noexcept
{
    return update_manifest;
}

} // namespace updated

