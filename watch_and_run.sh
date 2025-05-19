#!/bin/bash

# This is a script to automatically rebuild and run a program when a file changes. Feel free to change the executable to the one you want to run.
EXECUTABLE=./build/gui_test

# Initial build
cmake --build build

while true; do
    # Run the executable in the background
    $EXECUTABLE &
    PID=$!
    # Wait for a file change in src/ or include/
    fswatch -1 src include
    # Kill the running executable
    kill $PID
    wait $PID 2>/dev/null
    # Rebuild
    clear
    echo "Detected change, rebuilding..."
    cmake --build build
    # Loop will restart the executable
    sleep 0.5
done 