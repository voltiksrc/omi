#pragma once

#include <filesystem>
#include <vector>

bool install_staged_files(
    const std::filesystem::path &stage_dir,
    const std::filesystem::path &target_root,
    std::vector<std::filesystem::path> &installed_files
);

bool rollback_deployment(
    const std::filesystem::path &target_root,
    const std::vector<std::filesystem::path> &installed_files
);
