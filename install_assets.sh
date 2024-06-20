#!/bin/sh

# Define the target directory
TARGET_DIR="/home/$SUDO_USER/.config/gtklock/"
echo "$TARGET_DIR"

# Check if the gtklock directory does not exist
if [ ! -d "$TARGET_DIR" ]; then
    # Create the gtklock directory
    mkdir -p "$TARGET_DIR"
    echo "Created gtk directory"
fi

# Try to copy the assets and handle success or failure
if cp -r ../assets/* "$TARGET_DIR"; then
    echo "Successfully copied assets."
else
    echo "Failed to copy assets."
    exit 1 # Exit with an error code
fi
