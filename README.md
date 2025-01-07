# Osscz, an SSH Colouriser User Guide

Welcome to _osscz_, a command-line application designed to dynamically change
Ghostty's terminal themes based on SSH connections.

## Table of Contents

1. [Introduction](#introduction)
2. [System Requirements](#system-requirements)
3. [Installation](#installation)
4. [Configuration](#configuration)
5. [Basic Usage](#basic-usage)
6. [Troubleshooting](#troubleshooting)

## Introduction

_Osscz_ adjusts your terminal theme when connecting to different
SSH hosts. By specifying themes in your SSH configuration, you can have a
personalized terminal experience for each host.

## System Requirements

- **Operating System**: macOS
- **C++ Compiler**: Support for C++23 or later
- **Libraries**:
  - `fmt` for formatting
  - `spdlog` for logging
  - `toml11` for TOML parsing

## Installation

### Step 1: Prerequisites

Ensure you have Just, Conan v2.x, Cmake v3.30, and a C++ compiler and required
libraries installed. On Ubuntu, you can install them with:

Using Homebrew:

```sh
brew install fmt spdlog cmake conan just
```

### Step 2: Compile the Application

Clone the repository and compile the code:

```sh
git clone https://github.com/kontza/osscz.git
cd osscz
just cmake-release
# Binary is copied to $HOME/.local/bin/
```

### Step 3: Set Up Environment

The application uses environment variables for configuration:

- `GHOSTTY_RESOURCES_DIR`: Directory containing theme files. Usually set
  automatically by Ghostty. If it is not set, it is:
  - macOS: `/Applications/Ghostty.app/Contents/Resources/ghostty`
`
- `XDG_CONFIG_HOME`: Directory where `scz.toml` configuration file is located.
  Usually this is `$HOME/.config/`.

## Basic Usage

- **Change Theme**: Automatically changes terminal theme based on SSH host:
  - Create a shell script to work as a front-end for SSH. See `sch` in this repo
    for an example.
  - You need to add the following in your `~/ssh/config` for a host you want
    to change the theme:
    - With Ghostty:
      `SetEnv TERMINAL_THEME=[theme name from running 'ghostty +list-themes']`
    - With WezTerm:
      `SetEnv TERMINAL_THEME=[path to a WezTerm style color definition TOML file]`
    - A simple example:

      ```config
      Host alytollo
          SetEnv TERMINAL_THEME=$HOME/.config/wezterm/colors/3024.toml
      ```

- **Reset Theme**: To reset the theme manually, run:

  ```sh
  osscz RESET-SCHEME
  # Or
  osscz RESET-THEME
  ```

## Troubleshooting

- **Logs**:
  - Check logs for detailed debugging information. On macOS the location is `$TMPDIR/ssh_colouriser_[DATE].log`. From there you can check the parent command line to see if it matches some entry in your bypasses.
  - Note that both _Osscz_ processes, the main process, and the forked one, log partly the same log lines. Therefore, it is advisable to first `sort` the log file to get things in linear order.
