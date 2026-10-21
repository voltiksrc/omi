#include <iostream>
#include <string>
#include <string_view>

#include "install.hpp"
#include "output.hpp"
#include "sync.hpp"

using std::string;

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
    if (!sync_repositories()) {
        output::error("Repository syncing failed.");
        return 1;
    }
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

  } else if (command == "remove" || command == "rm") {
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
