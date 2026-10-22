
#pragma once

#include <filesystem>
#include <sqlite3.h>
#include <string>
#include <vector>

enum class InstallStatus { Installed, NotInstalled, Error };

class Database {
  public:
    explicit Database(const std::filesystem::path &path);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool initialize();

    InstallStatus is_installed(const std::string &name);

    bool list_installed();

    bool record_installation(const std::string &name, const std::string &version,
                             const std::vector<std::filesystem::path> &files);

    InstallStatus get_installed_files(const std::string &name,
                                      std::vector<std::filesystem::path> &files);

    bool remove_installation(const std::string &name);

  private:
    sqlite3 *db_ = nullptr;
};
