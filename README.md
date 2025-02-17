# Operating Systems Projects

This repository contains three assignments exploring fundamental Operating Systems concepts.

## Overview

### 1. Files Management
A program that lists and parses SF (Special File) files, extracting specific sections based on defined parameters.

**Features:**
- Lists files in a directory with specified name patterns and permissions.
- Parses SF files and verifies file structure.
- Extracts section content based on line numbers and offsets.

### 2. Thread and Process Synchronization
A multi-threaded application simulating synchronized operations between threads using mutexes, condition variables, and semaphores.

**Features:**
- Implements thread synchronization using mutexes and condition variables.
- Uses semaphores to coordinate thread execution order.
- Handles multiple child processes and thread synchronization.

### 3. Inter-Process Communication (IPC)
An implementation of IPC mechanisms using named pipes and shared memory for server-client communication.

**Features:**
- Uses named pipes (FIFOs) for request and response communication.
- Manages shared memory for inter-process data sharing.
- Handles file mapping and reading operations.
- Supports various commands for memory and file operations.

**Commands:**
- **Echo:** Server response check.
- **CREATE_SHM:** Create shared memory region.
- **WRITE_TO_SHM:** Write to shared memory.
- **MAP_FILE:** Map file to memory.
- **READ_FROM_FILE_OFFSET:** Read file data to shared memory.
- **EXIT:** Close connection.
