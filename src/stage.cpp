#include <filesystem>

#include "stage.hpp"
#include "recipe.hpp"
#include "output.hpp"
#include "process.hpp"

bool stage_source(const Package &info,
                  const std::filesystem::path &source_dir,
                  const std::filesystem::path &stage_dir) {
    namespace fs = std::filesystem;

    // Make DESTDIR absolute before passing it to the build system.
    fs::path absolute_path = fs::absolute(stage_dir);

    // Create the staging directory.
    fs::create_directories(absolute_path);

    // Pass DESTDIR to the child process.
    std::string destdir = "DESTDIR=" + absolute_path.string();

    if (info.build_system == "make") {
        if (run_command({"make", "install", "PREFIX=" + (absolute_path / "usr").string()}, source_dir, {destdir}) != 0) {
            output::error("Failed to stage Make files.");
            return false;
        }

    } else if (info.build_system == "meson") {
        if (run_command({"meson", "install", "-C", "build"},
                        source_dir, {destdir}) != 0) {
            output::error("Failed to stage Meson files.");
            return false;
        }

    } else if (info.build_system == "cmake") {
        if (run_command({"cmake", "--install", "build"},
                        source_dir, {destdir}) != 0) {
            output::error("Failed to stage CMake files.");
            return false;
        }

    } else {
        output::error("Build type unsupported.");
        return false;
    }

    return true;
}
