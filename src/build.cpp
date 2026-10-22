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

    } else if (info.build_system == "cargo") {
        output::info("Building with Cargo...");

        if (run_command({"cargo", "build", "--release"}, source_dir) != 0) {
            output::error("Cargo build failed.");
            return false;
        }
    } else if (info.build_system == "waf") {
        output::info("Building with Waf...");

        // Waf uses a configure step before compile
        if (run_command({"./waf", "configure", "--prefix=/usr"}, source_dir) != 0) {
            output::error("Waf configuration failed.");
            return false;
        }

        // Compile the package using its Waf build script
        if (run_command({"./waf", "build"}, source_dir) != 0) {
            output::error("Waf build failed.");
            return false;
        }
    } else if (info.build_system == "go") {
         output::info("Building with Go...");

         // Keep the compiled executable in its own directory
         fs::path build_dir = fs::absolute(source_dir / ".omi-go-build");

         fs::create_directories(build_dir);

         fs::path binary = build_dir / info.name;

         if (run_command({"go", "build", "-o", binary.string(), "."}, source_dir) != 0) {
             output::error("Go build failed.");
             return false;
         }
    } else {
        output::error("Unsupported build system: " + info.build_system);
        return false;
    }

    output::stage("Build completed successfully.");
    return true;
}
