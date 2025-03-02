#!/bin/bash
# Small helper script to build flatc, and put it in the bin directory

# Make sure this is run from the setup directory
if [ "$(pwd)" != "$PWD" ]; then
    echo "Please run this script from the setup directory"
    exit 1
fi

# Verify git is installed
if ! command -v git &> /dev/null; then
    echo "git could not be found"
    echo "Please install git and try again"
    exit 1
fi

# Verify cmake is installed
if ! command -v cmake &> /dev/null; then
    echo "cmake could not be found"
    echo "Please install cmake and try again"
    exit 1
fi

# Clone the flatbuffers repository
git clone https://github.com/google/flatbuffers.git

# No matter what, delete the flatbuffers directory if it exists
# at the end of the script past this point
trap "rm -rf flatbuffers" EXIT

# Build flatc
mkdir -p flatbuffers/build
(cd flatbuffers/build && cmake .. && make -j 8)

# Copy flatc to the bin directory
mkdir -p bin
cp flatbuffers/build/flatc ./bin/

echo "flatc built successfully"
