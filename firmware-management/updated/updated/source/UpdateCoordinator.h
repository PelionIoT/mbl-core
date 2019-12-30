#ifndef UPDATED_UPDATE_COORDINATOR_H
#define UPDATED_UPDATE_COORDINATOR_H

#include "Manifest.h"

#include <condition_variable>
#include <filesystem>
#include <mutex>
#include <string>

namespace updated {

class UpdateCoordinator final
{
public:
    UpdateCoordinator() = default;

    void start(const std::filesystem::path &payload_path, const std::string_view header_data);
    void run();

    Manifest get_manifest() const noexcept;

private:
    std::filesystem::path update_payload_path{ "/scratch" };
    std::mutex mutex;
    std::condition_variable condition_var;

    Manifest update_manifest;
};

} // namespace updated

#endif // UPDATED_UPDATE_COORDINATOR_H
