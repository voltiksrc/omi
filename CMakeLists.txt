
cmake_minimum_required(VERSION 3.20)

project(omi VERSION 0.3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(PkgConfig REQUIRED)
find_package(OpenSSL REQUIRED)

pkg_check_modules(CURL REQUIRED IMPORTED_TARGET libcurl)
pkg_check_modules(ARCHIVE REQUIRED IMPORTED_TARGET libarchive)
pkg_check_modules(TOMLPLUSPLUS REQUIRED IMPORTED_TARGET tomlplusplus)
pkg_check_modules(SQLITE3 REQUIRED IMPORTED_TARGET sqlite3)
pkg_check_modules(LIBSOLV REQUIRED IMPORTED_TARGET libsolv)
pkg_check_modules(LIBSOLVEXT REQUIRED IMPORTED_TARGET libsolvext)

add_executable(omi
    src/main.cpp
    src/recipe.cpp
    src/download.cpp
    src/sha256.cpp
    src/extract.cpp
    src/build.cpp
    src/install.cpp
    src/stage.cpp
    src/process.cpp
    src/sync.cpp
    src/database.cpp
    src/deploy.cpp
    src/search.cpp
    src/remove.cpp
    src/solver.cpp
)

target_link_libraries(omi PRIVATE
    PkgConfig::CURL
    PkgConfig::ARCHIVE
    PkgConfig::TOMLPLUSPLUS
    PkgConfig::SQLITE3
    PkgConfig::LIBSOLV
    PkgConfig::LIBSOLVEXT
    OpenSSL::Crypto
)

add_executable(omi-gen-solv
    tools/generate_solv.cpp
)

target_link_libraries(omi-gen-solv PRIVATE
    PkgConfig::LIBSOLV
    PkgConfig::TOMLPLUSPLUS
)

install(TARGETS omi RUNTIME DESTINATION bin)
