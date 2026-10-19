# omi

A Hybrid C++ package manager that uses TOML recipes and has a simple codebase.

## Development

It is an independent package manager that anyone can use, if you would like to help develop omi, then please contact me at: voltiksrc@gmail.com

## Features

- Source tarball downloads
- TOML Recipes
- SHA256 Verification
- Archive Extraction
- Source building (Which only supports CMake, Make, and Meson.)
- Install staging

## Dependencies

- libarchive
- libcurl
- libcrypto
- CMake
- toml++
- Ninja

## Build

```bash
git clone https://github.com/voltiksrc/omi.git
cd omi
```

```bash
meson setup build
meson compile -C build
```

## Install

```bash
sudo meson install -C build
```

## Usage

Binary install(later):

```bash
sudo omi -Fetch fastfetch --binary
```

Source install:

```bash
sudo omi -Fetch fastfetch
```

## Status

It's able to download a source tarball into a cache file, verify with SHA256, extract,
build it, and finally stage it.(Im almost at installation)
