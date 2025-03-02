# Setup Directory

This directory contains scripts and configuration for setting up the development environment.

## Scripts

### build_flatc.sh
Builds the `flatc` FlatBuffers compiler from source and places it in the `bin` directory.
- Verifies git and cmake are installed
- Clones the FlatBuffers repository
- Builds flatc using cmake
- Places the built binary in `setup/bin/`
- Automatically cleans up the FlatBuffers source after building

### reinstall_fb_headers.sh 
Reinstalls the FlatBuffers headers from the official repository.
- Verifies git is installed
- Clones the FlatBuffers repository
- Copies header files to ../include/flatbuffers/
- Automatically cleans up the FlatBuffers source after copying

## Environment Configuration

The parent directory contains a `.env` file that adds the `bin` directory to the system PATH.
You can use it via:
```
source .env
```
