
#include "build.hpp"
#include "output.hpp"
#include "process.hpp"

#include <filesystem>

// Builds the package using its specified build system.
bool build_source(const Package &info, const std::filesystem::path &source_dir) {
    namespace fs = std::filesystem;

    output::info("Build system: " + info.build_system);

    if (info.build_system == "make") {
        if (fs::exists(source_dir / "configure")) {
            output::info("Running configure...");

            if (run_command({"./configure", "--prefix=/usr"}, source_dir) != 0) {
                output::error("Make configure failed.");
                return false;
            }
        }

        output::info("Running make...");

        if (run_command({"make", "-j4"}, source_dir) != 0) {
            output::error("Make compile failed.");
            return false;
        }

    } else if (info.build_system == "meson") {
        output::info("Configuring with Meson...");

        if (run_command({"meson", "setup", "build", "--prefix=/usr"}, source_dir) != 0) {
            output::error("Meson setup failed.");
            return false;
        }

        output::info("Compiling with Meson...");

        if (run_command({"meson", "compile", "-C", "build"}, source_dir) != 0) {
            output::error("Meson compile failed.");
            return false;
        }

    } else if (info.build_system == "cmake") {
        output::info("Configuring with CMake...");

        if (run_command({
                "cmake", "-S", ".", "-B", "build",
                "-DCMAKE_INSTALL_PREFIX=/usr"
            }, source_dir) != 0) {
            output::error("CMake configure failed.");
            return false;
        }

        output::info("Compiling with CMake...");

        if (run_command({"cmake", "--build", "build", "--parallel"}, source_dir) != 0) {
            output::error("CMake build failed.");
            return false;
        }

    } else {
        output::error("Unsupported build system: " + info.build_system);
        return false;
    }

    std::cout<< "Build completed successfully.\n";
    return true;
}
