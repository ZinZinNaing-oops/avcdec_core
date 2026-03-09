# AVC Decoder Core (avcdec_core)

A comprehensive AVC (H.264) video decoder implementation with a Qt-based media player interface. This project combines the JM (Joint Model) reference decoder with a custom avcdec_core interface and a player application.

## Table of Contents

- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Installation](#installation)
  - [macOS Installation](#macos-installation)
  - [Windows Installation](#windows-installation)
- [Build Instructions](#build-instructions)
- [Running the Application](#running-the-application)

## Project Structure

```
avcdec_core/
├── JM/                    # JM reference decoder library
│   ├── ldecod/            # H.264 decoder implementation
│   └── lcommon/           # Common utilities
├── avcdec/                # avcdec_core interface library
│   ├── src/
│   │   └── AvcDecoder.cpp
│   └── include/
├── player_mdi/            # Qt-based media player
├── CMakeLists.txt         # Main CMake configuration
└── README.md
```

## Prerequisites

### macOS
- **OS**: macOS 10.13 or later
- **CMake**: Version 3.16 or higher
- **Compiler**: Clang/LLVM (included with Xcode)
- **Qt**: Qt5 (5.15+) or Qt6 (6.0+)

Install Xcode Command Line Tools:
```bash
xcode-select --install
```

### Windows
- **OS**: Windows 10 or later
- **CMake**: Version 3.16 or higher
- **Compiler**: MSVC 2019 or later (via Visual Studio)
- **Qt**: Qt5 (5.15+) or Qt6 (6.0+)

## Installation

### macOS Installation

#### 1. Install Homebrew (if not already installed)
```bash
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
```

#### 2. Install Dependencies
```bash
# Install CMake
brew install cmake

# Install Qt (choose one)
# For Qt6 
brew install qt@6

# OR for Qt5:
brew install qt@5
```

#### 3. Clone the Repository
```bash
git clone https://github.com/ZinZinNaing-oops/avcdec_core.git
cd avcdec_core
```

#### 4. Create Build Directory
```bash
mkdir build
cd build
```

#### 5. Configure with CMake
```bash
# For Qt6:
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" ..

# OR for Qt5:
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix qt@5)" ..
```

#### 6. Build
```bash
cmake --build . --config Release
```

### Windows Installation

#### 1. Install Visual Studio Build Tools
Download and install from [Visual Studio Community](https://visualstudio.microsoft.com/community/) with "Desktop development with C++" workload.

#### 2. Install CMake
Download from [cmake.org](https://cmake.org/download/) and install with "Add CMake to PATH" option checked.

#### 3. Install Qt
Download the offline installer from [Qt Download](https://www.qt.io/download-qt-installer):
- Choose Qt 6 (or Qt 5 if preferred)
- Select MSVC 2019 64-bit (or your MSVC version)
- Note the installation path (e.g., `C:\Qt\6.6.0`)

#### 4. Clone the Repository
```bash
git clone https://github.com/ZinZinNaing-oops/avcdec_core.git
cd avcdec_core
```

#### 5. Create Build Directory
```bash
mkdir build
cd build
```

#### 6. Configure with CMake
Open Command Prompt or PowerShell and run:

```batch
# For Qt6 (replace with your Qt path):
cmake -DCMAKE_PREFIX_PATH="C:\Qt\6.6.0\msvc2019_64" -G "Visual Studio 16 2019" -A x64 ..

# OR for Qt5 (replace with your Qt path):
cmake -DCMAKE_PREFIX_PATH="C:\Qt\5.15.0\msvc2019_64" -G "Visual Studio 16 2019" -A x64 ..
```

#### 7. Build
```bash
cmake --build . --config Release
```

## Build Instructions

### macOS
```bash
cd avcdec_core
mkdir build && cd build
cmake -DCMAKE_PREFIX_PATH="$(brew --prefix qt@6)" ..
cmake --build . --config Release -j4
```

### Windows
```batch
cd avcdec_core
mkdir build
cd build
cmake -DCMAKE_PREFIX_PATH="C:\Qt\6.6.0\msvc2019_64" -G "Visual Studio 16 2019" -A x64 ..
cmake --build . --config Release
```

## Running the Application

After successful build, find the executable:

### macOS
```bash
# From build directory
./player_mdi/player_mdi
# Or full path
open ./player_mdi/player_mdi.app
```

### Windows
```batch
# From build directory
.\player_mdi\Release\player_mdi.exe
```
