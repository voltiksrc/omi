# omi

A Hybrid C++ package manager that uses TOML recipes and has a relatively simple codebase.

## Development

It is an independent package manager that anyone can use, if you would
like to help develop omi, then please contact me at: tsukasatsa@gmail.com

## Features

- Source tarball downloads
- TOML Recipes
- SHA256 Verification
- Archive Extraction
- Source building which supports 6 types
- Install staging
- Merging into root

## Dependencies

- libarchive
- libcurl
- libcrypto
- CMake
- Toml++
- Ninja
- Make/Autotools
- Meson

## Recommended dependencies
- Go
- Cargo
- Waf

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

It's able to download a tarball, verify with sha256, extract, stage, install.
