
#include <cerrno>
#include <filesystem>
#include <fcntl.h>
#include <string>
#include <sys/stat.h>
#include <unistd.h>
#include <vector>

#include "deploy.hpp"
#include "output.hpp"

namespace fs = std::filesystem;

namespace {

constexpr const char *test_root = "/tmp/omi-test-root";

struct Entry {
    fs::path source;
    fs::path relative;
    fs::file_type type;
};

// Close file descriptors automatically
struct FD {
    int value = -1;

    explicit FD(int fd = -1) : value(fd) {}
    ~FD() {
        if (value >= 0)
            close(value);
    }

    FD(const FD &) = delete;
    FD &operator=(const FD &) = delete;

    operator int() const {
        return value;
    }
};

// No absolute paths or attempts to climb outside the root
bool safe_path(const fs::path &path) {
    if (path.empty() || path.is_absolute())
        return false;

    for (const auto &part : path) {
        if (part.empty() || part == "." || part == "..")
            return false;
    }

    return true;
}

// Keep everything inside our test root for now
int open_root(const fs::path &root) {
    if (fs::absolute(root).lexically_normal() != fs::path(test_root))
        return -1;

    if (mkdir(test_root, 0755) != 0 && errno != EEXIST)
        return -1;

    return open(test_root, O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);
}

// Walk through parent directories without following symlinks
// Optionally create missing directories
int open_parent(int rootfd, const fs::path &path, bool create) {
    FD current(dup(rootfd));

    if (current.value < 0)
        return -1;

    for (const auto &part : path.parent_path()) {
        std::string name = part.string();

        if (create && mkdirat(current, name.c_str(), 0755) != 0 &&
            errno != EEXIST)
            return -1;

        int next = openat(current, name.c_str(),
                          O_RDONLY | O_DIRECTORY | O_NOFOLLOW | O_CLOEXEC);

        if (next < 0)
            return -1;

        close(current.value);
        current.value = next;
    }

    int result = current.value;
    current.value = -1;
    return result;
}

// Check that a link stays inside its package directory tree
bool safe_link(const fs::path &relative, const fs::path &target) {
    if (target.empty() || target.is_absolute())
        return false;

    fs::path resolved = (relative.parent_path() / target).lexically_normal();

    return safe_path(resolved);
}

// Copy through file descriptors so destination symlinks aren't followed
bool copy_regular(const fs::path &source, int parent, const std::string &name,
                  const fs::path &installed,
                  std::vector<fs::path> &installed_files) {
    FD input(open(source.c_str(), O_RDONLY | O_NOFOLLOW | O_CLOEXEC));

    if (input.value < 0)
        return false;

    struct stat st {};

    if (fstat(input, &st) != 0 || !S_ISREG(st.st_mode))
        return false;

    FD output(openat(parent, name.c_str(),
                     O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW | O_CLOEXEC,
                     0600));

    if (output.value < 0)
        return false;

    // Track the new file right away, even if writing fails halfway
    installed_files.push_back(installed);

    char buffer[65536];

    while (true) {
        ssize_t count = read(input, buffer, sizeof(buffer));

        if (count == 0)
            break;

        if (count < 0) {
            if (errno == EINTR)
                continue;
            return false;
        }

        ssize_t written = 0;

        while (written < count) {
            ssize_t amount = write(output, buffer + written, count - written);

            if (amount < 0 && errno == EINTR)
                continue;

            if (amount <= 0)
                return false;

            written += amount;
        }
    }

    // Keep normal executable permissions, but not setuid/setgid bits
    return fchmod(output, st.st_mode & 0777) == 0;
}

// Check for conflicts before touching destination files
bool check_conflicts(int rootfd, const std::vector<Entry> &entries) {
    for (const auto &entry : entries) {
        FD parent(open_parent(rootfd, entry.relative, false));

        // Parent doesn't exist yet, so there can't be a file conflict
        if (parent.value < 0) {
            if (errno == ENOENT)
                continue;

            output::error("Unsafe destination: " + entry.relative.string());
            return false;
        }

        struct stat st {};
        std::string name = entry.relative.filename().string();

        if (fstatat(parent, name.c_str(), &st, AT_SYMLINK_NOFOLLOW) == 0) {
            // Existing directories are fine, existing files aren't
            if (entry.type != fs::file_type::directory || !S_ISDIR(st.st_mode)) {
                output::error("File conflict: " + entry.relative.string());
                return false;
            }
        } else if (errno != ENOENT) {
            output::error("Failed to check: " + entry.relative.string());
            return false;
        }
    }

    return true;
}

} // namespace

bool rollback_deployment(const fs::path &target_root,
                         const std::vector<fs::path> &installed_files) {
    FD root(open_root(target_root));

    if (root.value < 0)
        return false;

    bool success = true;

    // Undo files in reverse order, don't touch shared directories
    for (auto it = installed_files.rbegin(); it != installed_files.rend(); ++it) {
        fs::path relative = it->relative_path();

        if (!it->is_absolute() || !safe_path(relative)) {
            success = false;
            continue;
        }

        FD parent(open_parent(root, relative, false));

        if (parent.value < 0) {
            success = false;
            continue;
        }

        std::string name = relative.filename().string();
        struct stat st {};

        if (fstatat(parent, name.c_str(), &st, AT_SYMLINK_NOFOLLOW) != 0) {
            if (errno != ENOENT)
                success = false;
            continue;
        }

        // Never delete directories or special files here
        if (!S_ISREG(st.st_mode) && !S_ISLNK(st.st_mode)) {
            success = false;
            continue;
        }

        if (unlinkat(parent, name.c_str(), 0) != 0)
            success = false;
    }

    if (!success)
        output::error("Rollback incomplete");

    return success;
}

bool install_staged_files(const fs::path &stage_dir,
                          const fs::path &target_root,
                          std::vector<fs::path> &installed_files) {
    installed_files.clear();

    FD root(open_root(target_root));

    if (root.value < 0) {
        output::error("Could not open test root");
        return false;
    }

    std::vector<Entry> entries;

    try {
        if (!fs::is_directory(fs::symlink_status(stage_dir))) {
            output::error("Invalid staging directory");
            return false;
        }

        // Gather everything first so we can check before copying
        for (const auto &entry : fs::recursive_directory_iterator(stage_dir)) {
            fs::path relative = entry.path().lexically_relative(stage_dir);
            auto type = entry.symlink_status().type();

            if (!safe_path(relative)) {
                output::error("Unsafe staged path");
                return false;
            }

            if (type != fs::file_type::directory &&
                type != fs::file_type::regular &&
                type != fs::file_type::symlink) {
                output::error("Unsupported file type: " + relative.string());
                return false;
            }

            if (type == fs::file_type::symlink &&
                !safe_link(relative, fs::read_symlink(entry.path()))) {
                output::error("Unsafe symlink: " + relative.string());
                return false;
            }

            entries.push_back({entry.path(), relative, type});
        }

        if (!check_conflicts(root, entries))
            return false;

        // Everything looks good, start copying
        bool completed = true;

        for (const auto &entry : entries) {
            FD parent(open_parent(root, entry.relative, true));

            if (parent.value < 0) {
                output::error("Failed to open destination directory");
                completed = false;
                break;
            }

            std::string name = entry.relative.filename().string();
            bool success = false;

            if (entry.type == fs::file_type::directory) {
                success = mkdirat(parent, name.c_str(), 0755) == 0 ||
                          errno == EEXIST;

                // Make sure an existing entry is a real directory
                if (success) {
                    FD dir(openat(parent, name.c_str(),
                                  O_RDONLY | O_DIRECTORY |
                                  O_NOFOLLOW | O_CLOEXEC));

                    success = dir.value >= 0;
                }

            } else if (entry.type == fs::file_type::symlink) {
                fs::path target = fs::read_symlink(entry.source);

                if (safe_link(entry.relative, target)) {
                    success = symlinkat(target.c_str(), parent, name.c_str()) == 0;

                    if (success)
                        installed_files.push_back(fs::path("/") / entry.relative);
                }

            } else {
                success = copy_regular(entry.source, parent, name,
                                       fs::path("/") / entry.relative,
                                       installed_files);
            }

            if (!success) {
                output::error("Failed to deploy: " + entry.relative.string());
                completed = false;
                break;
            }

            if (entry.type != fs::file_type::directory)
                output::info("Installed " + entry.relative.string());
        }

        if (!completed) {
            output::stage("Rolling back deployment");

            // Release the root descriptor before opening it again
            close(root.value);
            root.value = -1;

            rollback_deployment(target_root, installed_files);
            return false;
        }

    } catch (const fs::filesystem_error &error) {
        output::error("Deployment failed: " + std::string(error.what()));

        close(root.value);
        root.value = -1;

        rollback_deployment(target_root, installed_files);
        return false;
    }

    output::info("Merged files into root successfully");
    return true;
}
