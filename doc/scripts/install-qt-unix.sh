#!/bin/bash

# Qt Unix-like Installation Script Wrapper
# Calls Python script for cross-platform compatibility

echo "========================================"
echo "Qt Unix-like Installation Script"
echo "========================================"
echo

# Check if Python is available
if ! command -v python3 &> /dev/null; then
    if ! command -v python &> /dev/null; then
        echo "[ERROR] Python is not installed or not in PATH"
        echo "[INFO] Please install Python 3.6 or higher"
        echo "[INFO] On Ubuntu/Debian: sudo apt-get install python3"
        echo "[INFO] On macOS: brew install python3"
        exit 1
    else
        PYTHON_CMD="python"
    fi
else
    PYTHON_CMD="python3"
fi

# Get script directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PYTHON_SCRIPT="$SCRIPT_DIR/install_qt.py"

# Check if Python script exists
if [ ! -f "$PYTHON_SCRIPT" ]; then
    echo "[ERROR] Python script not found: $PYTHON_SCRIPT"
    exit 1
fi

# Run Python script
echo "[INFO] Starting Qt installation using Python..."
$PYTHON_CMD "$PYTHON_SCRIPT"

# Check result
if [ $? -eq 0 ]; then
    echo "[SUCCESS] Installation completed successfully"
else
    echo "[ERROR] Installation failed"
fi

exit $?