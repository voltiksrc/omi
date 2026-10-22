
#include "output.hpp"

#include <curl/curl.h>
#include <toml++/toml.hpp>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

// Write the downloaded index data into our temporary file.
// Keep this static since the callback is only used in this file.
static size_t write_callback(char *ptr, size_t size, size_t nmemb,
                             void *userdata) {
    size_t total = size * nmemb;
    auto *file = static_cast<std::ofstream *>(userdata);

    file->write(ptr, static_cast<std::streamsize>(total));

    // Returning 0 tells libcurl that writing failed.
    if (!*file) {
        return 0;
    }

    return total;
}

bool sync_repositories() {
    // URL containing the repository's package index.
    const std::string url =
        "https://raw.githubusercontent.com/voltiksrc/omi-inventory/main/index.toml";

    // Keep the downloaded index separate until we know it's valid.
    const fs::path repo_dir = ".cache/repos/main";
    const fs::path index_path = repo_dir / "index.toml";
    const fs::path temp_path = repo_dir / "index.toml.tmp";

    // Create the repository cache directory if it doesn't exist.
    std::error_code ec;
    fs::create_directories(repo_dir, ec);

    if (ec) {
        output::error("Failed to create repository directory: " + ec.message());
        return false;
    }

    // Initialize libcurl for the index download.
    CURL *curl = curl_easy_init();

    if (!curl) {
        output::error("Failed to initialize libcurl.");
        return false;
    }

    // Open the temporary file instead of overwriting our existing index.
    std::ofstream file(temp_path, std::ios::binary | std::ios::trunc);

    if (!file) {
        output::error("Failed to open temporary index file.");
        curl_easy_cleanup(curl);
        return false;
    }

    // Set the download URL and tell libcurl where to write the data.
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

    // Follow redirects and treat HTTP errors as download failures.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    // Only allow HTTPS connections with valid certificates.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");

    // Don't let an unresponsive server hang Omi forever.
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);

    output::stage("Syncing repositories..");

    // Download the index into our temporary file.
    CURLcode result = curl_easy_perform(curl);

    // Check the output stream before closing it.
    bool write_failed = !file;
    file.close();

    if (file.fail()) {
        write_failed = true;
    }

    curl_easy_cleanup(curl);

    if (result != CURLE_OK || write_failed) {
        if (result != CURLE_OK) {
            output::error("Download failed: " +
                          std::string(curl_easy_strerror(result)));
        } else {
            output::error("Failed to write repository index.");
        }

        fs::remove(temp_path);
        return false;
    }

    // Parse the downloaded TOML before replacing the old index.
    try {
        auto index = toml::parse_file(temp_path.string());

        // Make sure the index contains the fields Omi expects.
        if (!index["repository"]["indexurl"].is_string() ||
            !index["packages"].is_table()) {
            output::error("Repository index is missing required fields.");
            fs::remove(temp_path);
            return false;
        }
    } catch (const toml::parse_error &error) {
        output::error("Invalid repository index: " +
                      std::string(error.what()));
        fs::remove(temp_path);
        return false;
    }

    // Replace the previous index only after downloading and validating it.
    fs::rename(temp_path, index_path, ec);

    if (ec) {
        output::error("Failed to update repository index: " + ec.message());
        fs::remove(temp_path);
        return false;
    }

    output::info("Repositories synced!");
    return true;
}
