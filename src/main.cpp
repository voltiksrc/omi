#include <iostream>
#include <string>
#include <string_view>

#include "database.hpp"
#include "install.hpp"
#include "output.hpp"
#include "sync.hpp"
#include "search.hpp"
#include "remove.hpp"

using std::string;

// version command
void smversion() {
    constexpr std::string_view version{"1.0"};
    std::cout << "omi version " << version << '\n';
}

// help command
void show_help() {
    std::cout << "Omi Package Manager\n\n";
    std::cout << "Usage: omi <command> [package]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  -Fetch <package>   Fetch and install a package\n";
    std::cout << "  remove <package>   Remove an installed package\n";
    std::cout << "  rm <package>       Alias for remove\n";
    std::cout << "  search <query>     Search repositories\n";
    std::cout << "  --list-installed   List installed packages\n";
    std::cout << "  --sync             Sync repositories\n";
    std::cout << "  --version          Show Omi version\n";
    std::cout << "  --help             Show this menu\n";
    std::cout << '\n';
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
    if (command == "--help" || command == "-h") {
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
    } else if (command == "--list-installed") {
        Database db("/tmp/omi-test-root/var/lib/omi/installed.db");

        if (!db.initialize()) {
            output::error("Failed to initialize database");
            return 1;
        }

        if (!db.list_installed()) {
            output::error("Failed to list installed packages");
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

    if (command == "-Fetch" || command == "-FETCH" || command == "-fetch") {
        for (int i = 2; i < argc; ++i) {
            if (!install_package(argv[i])) {
                return 1;
            }
        }

    } else if (command == "remove" || command == "rm") {
        for (int i = 2; i < argc; ++i) {
            if (!remove_package(argv[i])) {
                return 1;
            }
        }
        return 0;
    } else if (command == "search") {
        return search_packages(package) ? 0 : 1;
    } else { // check if its a proper command if not then quit with an error.
        std::cerr << "!!! Unknown command: " << argv[1] << ".\n";
        return 1;
    }
    return 0;
}
