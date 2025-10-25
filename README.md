# PROVE Lab - Mila (Electric Endurance Vehicle)
![Formatting Workflow](https://github.com/PROVELab/ProveMila-embedded/actions/workflows/check-format.yml/badge.svg)

Mila is an electric sports car with the goal of going 1000 miles on a single charge. This is the GitHub repository for the Low Voltage system. It's built on [PlatformIO](https://platformio.org/).

## Project Structure
The `src` folder contains the source code for all the embedded components of the system. Inside the `src` folder, there are folders for each node of the system and a `common` folder for shared code and drivers. See the READMEs within each folder for further information about each node.

## Dependencies
- Platformio (can be provided by platformio extension on vscode)
- clang-format
You can actually get both of these via uv!
Install [UV](https://docs.astral.sh/uv/getting-started/installation/)
Then follow the platformio/clang-format instructions to install both via uv. UV is the best!

## Platformio
You can easily install platformio via:
```
uv tool install platformio
```

## Formatting
To format all c/cpp files in the repo (src and include), run:
```
./format.sh
```
This will not affect lib (since it's external libraries)

### Git Pre-Commit Hook
A pre-commit hook is available to automatically check and apply formatting before each commit. To install the hook, run:
```
./install-hooks.sh
```

This will:
1. Format your C/C++ code automatically before each commit
2. Re-stage the formatted files so changes aren't lost
3. Allow commits to proceed with a warning if clang-format isn't installed

#### Installing clang-format
The formatting tools require clang-format to be installed on your system:
- **macOS**: `brew install clang-format`
- **Ubuntu/Debian**: `sudo apt-get install clang-format`
- **Windows with Chocolatey**: `choco install llvm`

Even better: use uv!
```
uv tool install clang-format platformio
```
