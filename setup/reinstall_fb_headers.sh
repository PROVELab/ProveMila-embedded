#!/bin/bash

# Small helper script to reinstall the flatbuffers headers
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

# Clone the flatbuffers repository
git clone https://github.com/google/flatbuffers.git

# No matter what, delete the flatbuffers directory if it exists
# at the end of the script past this point
trap "rm -rf flatbuffers" EXIT

# Copy the headers to the include directory
cp flatbuffers/include/flatbuffers/*.h ../include/flatbuffers/

echo "Flatbuffers headers reinstalled successfully"
