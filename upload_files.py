#!/usr/bin/env python3
import os
import sys
import platform
import subprocess
import time

def run_command(command, description):
    print(f"\n=== {description} ===")
    try:
        subprocess.run(command, check=True)
        print(f"✓ {description} completed successfully!")
        return True
    except subprocess.CalledProcessError as e:
        print(f"✗ Error during {description}: {e}")
        return False

def deploy():
    # Get the current directory
    current_dir = os.path.dirname(os.path.abspath(__file__))
    
    # Check if platformio is installed
    try:
        subprocess.run(['platformio', '--version'], check=True, capture_output=True)
    except subprocess.CalledProcessError:
        print("Error: PlatformIO is not installed or not in PATH")
        sys.exit(1)

    # Step 1: Build the project
    if not run_command(['platformio', 'run'], "Building project"):
        return False

    # Step 2: Upload filesystem
    if not run_command(['platformio', 'run', '-t', 'uploadfs'], "Uploading filesystem"):
        return False

    # Step 3: Upload firmware
    if not run_command(['platformio', 'run', '-t', 'upload'], "Uploading firmware"):
        return False

    # Step 4: Monitor serial output
    print("\n=== Starting Serial Monitor ===")
    print("Press Ctrl+C to exit")
    try:
        subprocess.run(['platformio', 'device', 'monitor'], check=True)
    except KeyboardInterrupt:
        print("\nSerial monitor stopped")
    except subprocess.CalledProcessError as e:
        print(f"Error starting serial monitor: {e}")

if __name__ == "__main__":
    deploy() 