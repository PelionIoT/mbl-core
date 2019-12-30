#include "UpdateCoordinator.h"

#include <cassert>
#include <filesystem>
#include <iostream>
#include <string>

namespace updated {

void UpdateCoordinator::start(const std::filesystem::path &payload_path, std::string_view header_data)
{
    assert(payload_path && header_data);
    update_manifest.header = header_data;
    update_payload_path /= payload_path.filename();
    std::filesystem::create_hard_link(payload_path, update_payload_path);
    std::unique_lock<std::mutex> ul{ mutex };
    //TODO: set global update state to UPDATING
    ul.unlock();
    condition_var.notify_all();
}

void UpdateCoordinator::run()
{
    std::unique_lock<std::mutex> ul{ mutex };
    // TODO: query UpdateTracker state to guard against spurious wakeups
    condition_var.wait(ul, [](){ return true; });
    // TODO: call swupdate
    std::cout << "call swupdate" << '\n';
    ul.unlock();
    condition_var.notify_all();
}

void UpdateCoordinator::get_manifest() const noexcept
{
    return update_manifest;
}

} // namespace updated

