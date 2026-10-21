#pragma once

#include <filesystem>
#include <sqlite3.h>
#include <string>
#include <vector>

class Database {
public:
    explicit Database(const std::filesystem::path &path);
    ~Database();

    Database(const Database &) = delete;
    Database &operator=(const Database &) = delete;

    bool initialize();
    bool is_installed(const std::string &name);

    bool record_installation(
        const std::string &name,
        const std::string &version,
        const std::vector<std::filesystem::path> &files
    );

private:
    sqlite3 *db_ = nullptr;
};
