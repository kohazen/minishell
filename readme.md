# Mini UNIX Shell - User Documentation

## Table of Contents
1. [Introduction](#introduction)
2. [Installation](#installation)
3. [Getting Started](#getting-started)
4. [Features](#features)
5. [Command Syntax](#command-syntax)
6. [Examples](#examples)
7. [Error Handling](#error-handling)
8. [Testing](#testing)
9. [Troubleshooting](#troubleshooting)

---

## 1. Introduction

**myshell** is a lightweight UNIX-style command-line shell implemented in C. It provides essential shell functionality including external program execution, I/O redirection, pipelines, background job execution, and built-in commands. The shell is designed to be simple, robust, and educational, demonstrating core operating system concepts such as process management, file descriptors, and signal handling.

### Key Capabilities
- Execute external programs with PATH resolution
- Parse arguments including quoted strings
- Input and output redirection
- Single-stage pipelines
- Background process execution
- Built-in `cd` and `exit` commands
- Command sequencing with semicolons
- Robust error handling

---

## 2. Installation

### Prerequisites
- GCC compiler
- UNIX-like operating system (Linux, macOS, BSD)
- Make utility

### Build Instructions

1. **Extract the archive:**
   ```bash
   tar -xzf myshell.tar.gz
   cd myshell
   ```

2. **Compile the shell:**
   ```bash
   make
   ```
   This generates the `myshell` executable in the current directory.

3. **Clean build artifacts (optional):**
   ```bash
   make clean
   ```

### Verification
After building, verify the executable exists:
```bash
ls -l myshell
./myshell
```

---

## 3. Getting Started

### Launching the Shell

To start myshell, simply run:
```bash
./myshell
```

You will see the prompt:
```
myshell>
```

### Exiting the Shell

Use the `exit` command or press `Ctrl-D` (EOF):
```
myshell> exit
```

### Basic Command Execution

Type any command available in your system's PATH:
```
myshell> ls -l
myshell> pwd
myshell> date
```

---

## 4. Features

### 4.1 External Program Execution

myshell can execute any program found in your system's PATH environment variable. It uses `execvp()` for PATH resolution.

**Example:**
```
myshell> ls
myshell> grep pattern file.txt
myshell> python3 script.py
```

### 4.2 Argument Parsing

The shell supports multiple argument formats:

**Unquoted arguments:**
```
myshell> echo hello world
```

**Double-quoted strings** (preserves spaces):
```
myshell> echo "hello world"
```

**Single-quoted strings:**
```
myshell> echo 'hello world'
```

### 4.3 Input Redirection (`<`)

Redirect a file's contents to a command's standard input:

**Syntax:** `command < input_file`

**Example:**
```
myshell> sort < unsorted.txt
myshell> wc -l < data.txt
```

### 4.4 Output Redirection (`>`)

Redirect a command's standard output to a file (overwrites if exists):

**Syntax:** `command > output_file`

**Example:**
```
myshell> echo "Hello" > greeting.txt
myshell> ls -l > directory_listing.txt
```

### 4.5 Pipelines (`|`)

Connect the output of one command to the input of another:

**Syntax:** `command1 | command2`

**Example:**
```
myshell> ls -l | grep txt
myshell> cat file.txt | sort | uniq
myshell> ps aux | grep python
```

**Pipeline with redirection:**
```
myshell> cat data.txt | grep pattern > results.txt
```

### 4.6 Background Execution (`&`)

Run commands in the background, allowing the shell to accept new commands immediately:

**Syntax:** `command &`

**Example:**
```
myshell> sleep 10 &
[bg] pid 12345
myshell>
```

The shell prints `[bg] pid <PID>` to confirm background execution.

### 4.7 Built-in Commands

#### `cd` - Change Directory

Navigate the filesystem without spawning a new process.

**Syntax:**
- `cd <directory>` - Change to specified directory
- `cd` - Change to home directory

**Examples:**
```
myshell> cd /home/user
myshell> pwd
/home/user

myshell> cd ..
myshell> cd
```

#### `exit` - Exit Shell

Terminate the shell session.

**Syntax:** `exit`

### 4.8 Command Sequencing (`;`)

Execute multiple commands in sequence on a single line:

**Syntax:** `command1 ; command2 ; command3`

**Example:**
```
myshell> echo "First" ; echo "Second" ; echo "Third"
First
Second
Third

myshell> cd /tmp ; ls ; pwd
```

### 4.9 Signal Handling

- **Ctrl-C (SIGINT):** Does not terminate the shell, only interrupts foreground processes
- **Background process management:** Automatic cleanup of zombie processes via SIGCHLD handler

---

## 5. Command Syntax

### General Syntax
```
command [arguments] [< input_file] [> output_file] [| command2] [&]
```

### Operator Precedence and Parsing Rules

1. **Semicolon (`;`)** separates distinct commands
2. **Pipe (`|`)** has higher precedence within a single command
3. **Redirection (`<`, `>`)** applies to the immediate command
4. **Background (`&`)** must appear at the end of a command

### Valid Command Patterns

```bash
# Simple command
command arg1 arg2

# With redirection
command < input.txt > output.txt

# Pipeline
cmd1 | cmd2

# Pipeline with redirection
cmd1 < in.txt | cmd2 > out.txt

# Background
command &

# Sequencing
cmd1 ; cmd2 ; cmd3

# Complex combination
cmd1 arg1 > temp.txt ; cat temp.txt | cmd2 ; cmd3 &
```

---

## 6. Examples

### Example 1: File Operations
```bash
myshell> echo "Line 1" > file.txt
myshell> echo "Line 2" >> file.txt    # Note: >> not supported, will overwrite
myshell> cat file.txt
Line 2
```

### Example 2: Text Processing Pipeline
```bash
myshell> cat names.txt | sort | uniq > sorted_names.txt
myshell> wc -l < sorted_names.txt
```

### Example 3: System Monitoring
```bash
myshell> ps aux | grep myshell > my_processes.txt
myshell> cat my_processes.txt
```

### Example 4: Directory Navigation
```bash
myshell> pwd
/home/user
myshell> cd /tmp
myshell> ls > /tmp/contents.txt
myshell> cd
myshell> pwd
/home/user
```

### Example 5: Background Jobs
```bash
myshell> sleep 30 &
[bg] pid 5678
myshell> echo "Shell is still responsive"
Shell is still responsive
```

### Example 6: Data Sorting
```bash
myshell> printf "cherry\napple\nbanana\n" > fruits.txt
myshell> sort < fruits.txt
apple
banana
cherry
```

### Example 7: Command Chaining
```bash
myshell> mkdir testdir ; cd testdir ; touch file1.txt file2.txt ; ls
file1.txt
file2.txt
```

---

## 7. Error Handling

myshell provides clear error messages for common mistakes:

### Syntax Errors

**Missing filename after redirection:**
```
myshell> ls >
syntax error: expected filename after '>'
```

**Misplaced pipe:**
```
myshell> | grep pattern
syntax error: misplaced pipe
```

**Empty pipe:**
```
myshell> cat file.txt |
syntax error: misplaced pipe
```

### Runtime Errors

**Command not found:**
```
myshell> nonexistent_command
exec failed: nonexistent_command: No such file or directory
```

**File access errors:**
```
myshell> cat nonexistent.txt
cat: nonexistent.txt: No such file or directory
```

**cd to invalid directory:**
```
myshell> cd /invalid/path
cd: No such file or directory
```

---

## 8. Testing

### Running the Test Suite

A comprehensive test suite is provided in `tests/run_tests.sh`:

```bash
# Make script executable
chmod +x tests/run_tests.sh

# Run all tests
tests/run_tests.sh
```

### Test Coverage

The test suite validates:
1. Basic echo functionality
2. Quoted argument parsing
3. Output redirection
4. Pipelines with sorting
5. Input redirection
6. Background execution
7. File overwriting behavior
8. Syntax error handling
9. Built-in cd command
10. Complex pipe and redirection combinations
11. Command sequencing

### Expected Output
```
Running comparisons...
[PASS] 01_echo.txt
[PASS] 02_quoted.txt
[PASS] 03_redir.txt
...
[PASS] 12_semicolon.txt

Summary: 12 passed, 0 failed (total 12)
```

---

## 9. Troubleshooting

### Common Issues

**Q: Shell doesn't start**
```
A: Ensure the binary has execute permissions:
   chmod +x myshell
```

**Q: Commands not found**
```
A: Check your PATH environment variable:
   myshell> echo $PATH
   Ensure required directories are included.
```

**Q: Compilation errors**
```
A: Verify you have GCC installed:
   gcc --version
   
   Ensure all source files are present:
   ls myshell.c Makefile
```

**Q: Background processes become zombies**
```
A: myshell includes a SIGCHLD handler that automatically 
   reaps zombie processes. If issues persist, check system
   resource limits.
```

**Q: Ctrl-C kills the shell**
```
A: This shouldn't happen - myshell installs a SIGINT handler.
   If it does occur, there may be a signal handling issue.
```

**Q: Redirection not working**
```
A: Ensure you have write permissions in the target directory.
   Check file permissions:
   ls -l output_file.txt
```

### Limitations

- **No multi-stage pipelines:** Only single pipe (cmd1 | cmd2) supported
- **No append redirection:** `>>` operator not implemented (use `>` only)
- **No environment variable expansion:** `$VAR` syntax not supported
- **No command history:** Up/down arrows don't recall previous commands
- **No wildcards:** Globbing patterns like `*.txt` not expanded by shell
- **No job control:** `fg`, `bg`, `jobs` commands not available

### Getting Help

For issues not covered here:
1. Review the technical report for implementation details
2. Check the source code comments in `myshell.c`
3. Examine test cases in `tests/run_tests.sh`

---

## Appendix: Quick Reference

### Built-in Commands
| Command | Description |
|---------|-------------|
| `cd [dir]` | Change directory |
| `exit` | Exit shell |

### Operators
| Operator | Description | Example |
|----------|-------------|---------|
| `<` | Input redirection | `sort < input.txt` |
| `>` | Output redirection | `ls > output.txt` |
| `\|` | Pipeline | `cat file \| grep pattern` |
| `&` | Background | `sleep 10 &` |
| `;` | Command separator | `cmd1 ; cmd2` |

### Special Keys
| Key | Action |
|-----|--------|
| `Ctrl-C` | Interrupt foreground process |
| `Ctrl-D` | Exit shell (EOF) |

---

**Version:** 1.0  
**Last Updated:** December 2025  
**Author:** Saurabh Pandey (2025MCS2151), Maj Girish Singh Thakur (2025MCS2973)