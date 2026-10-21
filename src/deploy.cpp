#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

bool install_staged_files(const fs::path &stage_dir,
                          const fs::path &target_root) {
    for (const auto &entry : fs::recursive_directory_iterator(stage_dir)) {
        if (entry.is_regular_file()) {
            fs::path relative = entry.path().lexically_relative(stage_dir);
            fs::path destination = target_root / relative;

            if (fs::exists(destination) || fs::is_symlink(destination)) {
                std::cerr<<"File conflict: "<<destination<<'\n';
                return false;
            }

            fs::create_directories(destination.parent_path());
            fs::copy_file(entry.path(), destination);

            std::cout << entry.path() << " -> " << destination << '\n';
        }
    }

    std::cout<<">> Merged files into root successfully.\n";

    return true;
}
