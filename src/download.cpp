#include <algorithm>
#include <curl/curl.h>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <string_view>

#include "download.hpp"

using std::string;

// DEBI means download, extract, build, and install.

// Progress state
struct Progress {
    int last_percentage = -1;
};

// Progress bar

int progress_callback(void *clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal,
                      curl_off_t ulnow) {
    auto *progress = static_cast<Progress *>(clientp);

    if (dltotal > 0) {
        constexpr int bar_width = 22;

        // Calculate the percentage and how much of the bar is filled.
        int percentage = static_cast<double>(dlnow) / dltotal * 100;
        int filled = percentage * bar_width / 100;

        if (percentage != progress->last_percentage) {
            std::cout << "\r[";

            // Print the completed part of the bar.
            for (int i = 0; i < bar_width; i++) {
                if (percentage == 100) {
                    std::cout << "━";
                } else if (i == std::max(0, filled - 1)) {
                    // Put the arrow at the end of the completed part.
                    std::cout << "❯";
                } else if (i < filled) {
                    std::cout << "━";
                } else {
                    // Print dots for the remaining download.
                    std::cout << "·";
                }
            }

            std::cout << "] " << percentage << "%" << std::flush;

            progress->last_percentage = percentage;
        }
    }

    return 0;
}

// Main libcurl function that writes our data to a file so we can DEBI
// peacefully.
static size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t total = size * nmemb;

    auto *file = static_cast<std::ofstream *>(userdata);

    file->write(ptr, static_cast<std::streamsize>(total));

    if (!*file) {
        return 0;
    }

    return total;
}

string download_source(const string &source_url, const string &name, const string &version) {
    // Initialize libcurl so we can download and write data
    // using its functions.
    CURL *curl = curl_easy_init();
    if (!curl) {
        std::cerr << "Failed to initalize libcurl.\n";
        return "";
    }

    string extension;

    for (const std::string_view ext :
         {".tar.gz", ".tar.xz", ".tar.bz2", ".tar.zst", ".zip", ".tar.lz"}) {
        if (source_url.find(ext) != string::npos) {
            extension = ext;
            break;
        }
    }

    string output_path = ".cache/tbdownload/" + name + "-" + version + extension;

    // Create .cache/tarballdownload so it never fails to Download the tarballs.
    std::filesystem::create_directories(".cache/tbdownload");
    std::ofstream file(output_path, std::ios::binary);
    if (!file) {
        std::cerr << "Failed to open output file\n";
        curl_easy_cleanup(curl);
        return "";
    }

    // Configure the source URL for the download.
    curl_easy_setopt(curl, CURLOPT_URL, source_url.c_str());

    // Register our callback to write incoming data.
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);

    // Pass the output file to our callback.
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &file);

    // Follow HTTP redirects.
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    // tarballdownload HTTP error responses (ownload the tarballsh as 404) as failures.
    curl_easy_setopt(curl, CURLOPT_FAILONERROR, 1L);

    Progress progress{};
    // Enable progress tracking
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);

    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, &progress);

    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);

    // Enable timeouts when a url hangs
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 120L);

    // Require HTTPS and verify TLS certificates.
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(curl, CURLOPT_PROTOCOLS_STR, "https");
    curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS_STR, "https");

    CURLcode result = curl_easy_perform(curl);

    if (result == CURLE_OK && progress.last_percentage < 100) {
        std::cout << "\r[";

        for (int i = 0; i < 22; ++i) {
            std::cout << "━";
        }

        std::cout << "] 100%" << std::flush;
    }

    if (progress.last_percentage >= 0 || result == CURLE_OK) {
        std::cout << '\n';
    }

    std::cout << '\n';
    // long http_code{0};
    // curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code
    // print a newline and make sure to exit if the download fails.
    file.close();

    if (result != CURLE_OK) {
        std::cerr << "Download failed: " << curl_easy_strerror(result) << '\n';
        // std::cout << "HTTP status: " << http_code << '\n';
        std::filesystem::remove(output_path);
        curl_easy_cleanup(curl);
        return "";
    }
    // free resources so we dont memory leak
    curl_easy_cleanup(curl);
    return output_path;
}
