#include "database.hpp"

#include <iostream>
#include <stdexcept>

Database::Database(const std::filesystem::path &path) {
    int result = sqlite3_open(path.c_str(), &db_);

    if (result != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        db_ = nullptr;

        throw std::runtime_error("Failed to open database: " + error);
    }
}

Database::~Database() {
    if (db_) {
        sqlite3_close(db_);
    }
}

bool Database::initialize() {
    if (sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to enable foreign keys: " << sqlite3_errmsg(db_) << '\n';
        return false;
    }

    // Create the two tables: packages and files, they both will hold
    // metadata for installed packages.
    const char *sql = R"(
      CREATE TABLE IF NOT EXISTS packages (
          id INTEGER PRIMARY KEY,
          name TEXT NOT NULL UNIQUE,
          version TEXT NOT NULL
      );

      CREATE TABLE IF NOT EXISTS files (
          path TEXT PRIMARY KEY,
          package_id INTEGER NOT NULL,
          FOREIGN KEY (package_id) REFERENCES packages(id)
      );
  )";

    int result = sqlite3_exec(db_, sql, nullptr, nullptr, nullptr);
    if (result != SQLITE_OK) {
        std::cerr << sqlite3_errmsg(db_) << '\n';
        return false;
    }

    return true;
}

bool Database::is_installed(const std::string &name) {
    const char *sql = "SELECT 1 FROM packages WHERE name = ? LIMIT 1;";

    sqlite3_stmt *stmt = nullptr;

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << sqlite3_errmsg(db_) << '\n';
        return false;
    }

    if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        std::cerr << sqlite3_errmsg(db_) << '\n';
        sqlite3_finalize(stmt);
        return false;
    }

    int result = sqlite3_step(stmt);

    if (result == SQLITE_ROW) {
        std::cout << "A matching package was found: " << name.c_str() << '\n';
    } else if (result == SQLITE_DONE) {
        std::cout << "No matching packages found.\n";

    } else {
        std::cerr << "Database query failed: " << sqlite3_errmsg(db_) << '\n';
    }

    sqlite3_finalize(stmt);
    return result == SQLITE_ROW;
}

bool Database::record_installation(const std::string &name, const std::string &version,
                                   const std::vector<std::filesystem::path> &files) {
    if (sqlite3_exec(db_, "BEGIN TRANSACTION;", nullptr, nullptr, nullptr) != SQLITE_OK) {
        std::cerr << "Failed to begin transaction: "<< sqlite3_errmsg(db_) << '\n';
        return false;
    }

    sqlite3_stmt *stmt = nullptr;

    const char *sql = "INSERT INTO packages (name, version) VALUES (?, ?);";    

    if (sqlite3_prepare_v2(db_, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        std::cerr << sqlite3_errmsg(db_) << '\n';
        return false;
    }

    if (sqlite3_bind_text(stmt, 1, name.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        std::cerr<<sqlite3_errmsg(db_)<<'\n';
        return false;
    }

    if (sqlite3_bind_text(stmt, 2, version.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK) {
        std::cerr<<sqlite3_errmsg(db_)<<'\n';
        return false;
    }

    int result = sqlite3_step(stmt);

    if (result != SQLITE_DONE) {
        std::cout<<"Failed to insert metadata into db.\n";
    } else if (result == SQLITE_DONE) {
        std::cout<<"Successfully inserted metadata into db.\n";
    }
    sqlite3_int64 package_id = sqlite3_last_insert_rowid(db_);
    return false;
}
