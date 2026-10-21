#pragma once

#include <filesystem>

bool install_staged_files(const std::filesystem::path &stage_dir,
                          const std::filesystem::path &target_root);
