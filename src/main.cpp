#include <filesystem>
#include <iostream>
#include <string>
#include <string_view>

#include "build.hpp"
#include "download.hpp"
#include "extract.hpp"
#include "install.hpp"
#include "output.hpp"
#include "recipe.hpp"
#include "sha256.hpp"
#include "sync.hpp"

using std::string;
namespace fs = std::filesystem;

bool is_source_root(const fs::path &path) {
  return fs::exists(path / "meson.build") ||
         fs::exists(path / "CMakeLists.txt") ||
         fs::exists(path / "configure") || fs::exists(path / "Makefile");
}

// version command
void smversion() {
  constexpr std::string_view version{"0.50"};
  std::cout << "omi version " << version << '\n';
}

// help command
void show_help() {
  std::cout << "Omi Package manager help\n";
  std::cout << '\n';
  std::cout << "Example: sudo omi -Fetch fastfetch\n";
  std::cout << "Options:\n";
  std::cout << "omi -Fetch <package>\n";
  std::cout << "omi remove <package>\n";
  std::cout << "omi search <package>\n";
  std::cout << "omi --help\n";
  std::cout << "omi --sync\n";
}

bool install_package(const string &package) {
  std::string recipe_path = "recipes/" + package + "/package.toml";

  if (!fs::is_regular_file(recipe_path)) {
    std::cerr << "Package not found: " << package << '\n';
    return 0;
  }

  Package info = read_recipe(recipe_path);

  // Checks if any of the stages are empty and if it is we exit before doing the
  // DEBI steps.
  if (info.name.empty() || info.version.empty() || info.url.empty()) {
    output::error("Invalid or incomplete package recipe.");
    return 1;
  }

  // Download the package and save the archive path so we can use it later.
  output::stage("Downloading " + info.name + "...");
  string archive = download_source(info.url, info.name, info.version);

  if (archive.empty()) {
    return 1;
  }

  // SHA256 call
  output::stage("Verifying SHA-256");

  if (!verify_sha256(archive, info.sha256)) {
    return 1;
  }

  // Extract the downloaded archive after verifying its SHA-256.
  output::stage("Extracting sources");

  if (!extract_source(archive, info.name, info.version, ".cache/build")) {
    output::error("Extraction failed.");
    return 1;
  }

  // Find the source directory.
  fs::path extract_dir =
      fs::path(".cache/build") / (info.name + "-" + info.version);
  fs::path source_dir;

  if (is_source_root(extract_dir)) {
    source_dir = extract_dir;
  } else {
    for (const auto &entry : fs::directory_iterator(extract_dir)) {
      if (entry.is_directory() && is_source_root(entry.path())) {
        source_dir = entry.path();
        break;
      }
    }
  }

  if (source_dir.empty()) {
    output::error("Could not locate package source directory.\n");
    return 1;
  }

  // build the package after extracting.
  output::stage("Building " + info.name + "-" + info.version);
  if (!build_source(info, source_dir)) {
    return 1;
  }

  // Stage the compiled package.
  fs::path stage_dir =
      fs::path(".cache/stage") / (info.name + "-" + info.version);

  output::stage("Staging " + info.name + "-" + info.version);
  if (!install_source(info, source_dir, stage_dir)) {
    output::error("Installation staging failed.");
    return 1;
  }
  output::info("Files staged successfully.");
  return true;
}

int main(int argc, char *argv[]) {
  // We need to check before declaring the command as argv[1].
  // If we dont then running omi without arguments could cause an error.
  if (argc < 2) {
    std::cout << "Please enter a command and optionally a package.\n";
    return 1;
  }

  string command = argv[1];

  // command line interactions so commands actually do something
  if (command == "--help") {
    show_help();
    return 0;
  } else if (command == "--version") {
    smversion();
    return 0;
  } else if (command == "--sync") {
    sync_repositories();
    return 0;
  }

  // Make sure they enter a package so we can safely assign
  // argv[2] to the package variable placeholder.
  if (argc < 3) {
    std::cout << "Usage: omi <command> [package]\n";
    return 1;
  }

  // Since we confirmed that argv[2] exists, we can declare it as the main
  // package.
  string package = argv[2];

  if (command == "-Fetch") {
    for (int i = 2; i < argc; ++i) {
      if (!install_package(argv[i])) {
        return 1;
      }
    }

  } else if (command == "remove") {
    std::cout << ">>> Removing " << package << "..\n";
    return 0;

  } else if (command == "search") {
    std::cout << ">>> Searching for " << package << "...\n";
    std::cout << ">>> Found package: " << package << ".\n";
  } else { // check if its a proper command if not then quit with an error.
    std::cerr << "!!! Unknown command: " << argv[1] << ".\n";
    return 1;
  }
  return 0;
}
