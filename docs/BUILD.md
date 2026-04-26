# Build and Development Guide

A comprehensive guide for building, testing, and developing the Proxy Server.

## Table of Contents

1. [Prerequisites](#prerequisites)
2. [Quick Start](#quick-start)
3. [Build Targets](#build-targets)
4. [Development Setup](#development-setup)
5. [Testing](#testing)
6. [Debugging](#debugging)
7. [Performance Analysis](#performance-analysis)
8. [Troubleshooting](#troubleshooting)

## Prerequisites

### Linux/Ubuntu

```bash
sudo apt-get update
sudo apt-get install -y \
    build-essential \
    gcc \
    make \
    git \
    gdb \
    valgrind \
    cppcheck
```

### macOS

```bash
brew install gcc make git lldb valgrind
```

### CentOS/RHEL

```bash
sudo yum groupinstall -y "Development Tools"
sudo yum install -y gcc make git gdb valgrind
```

### Verification

```bash
gcc --version
make --version
git --version
gdb --version
```

## Quick Start

### Build

```bash
git clone https://github.com/M-Taha-Raza12/proxy-Server-.git
cd proxy-Server-
make
```

### Run

```bash
./proxy-server -p 8080 -v
```

### Test

```bash
curl -x http://localhost:8080 http://example.com
```

## Build Targets

### Standard Targets

```bash
# Default build (optimized)
make

# Clean build artifacts
make clean

# Rebuild from scratch
make clean && make

# Install to system (requires sudo)
sudo make install

# Remove installed files
sudo make uninstall
```

### Development Targets

```bash
# Build with debug symbols
make DEBUG=1

# Build with all warnings
make WARN=1

# Build with optimization level 3
make OPT=3

# Static analysis
make check

# Format code (if clang-format installed)
make format
```

### Test Targets

```bash
# Run tests (if test suite exists)
make test

# Memory leak check
make memcheck

# Verbose output during build
make VERBOSE=1

# Build with coverage
make coverage
```

## Development Setup

### Recommended IDE/Editor Setup

#### VS Code

**Install extensions:**
- C/C++ (Microsoft)
- Makefile Tools
- CodeLLDB (debugger)

**.vscode/settings.json:**
```json
{
    "C_Cpp.default.compilerPath": "/usr/bin/gcc",
    "C_Cpp.codeAnalysisRunOnSave": true,
    "editor.formatOnSave": true,
    "[c]": {
        "editor.defaultFormatter": "ms-vscode.cpptools"
    }
}
```

#### Vim

```bash
# Install vim-c-syntax plugin
git clone https://github.com/vim-scripts/c.vim.git ~/.vim/bundle/c.vim

# Configure .vimrc
echo "set cindent" >> ~/.vimrc
echo "set tabstop=8" >> ~/.vimrc
echo "set shiftwidth=8" >> ~/.vimrc
```

#### Emacs

```bash
echo "(setq c-default-style \"linux\")" >> ~/.emacs
echo "(setq-default c-basic-offset 8)" >> ~/.emacs
echo "(setq indent-tabs-mode t)" >> ~/.emacs
```

## Testing

### Manual Testing

#### Test 1: Basic Proxy

```bash
# Terminal 1: Start proxy
./proxy-server -p 8080

# Terminal 2: Test with curl
curl -x http://localhost:8080 http://httpbin.org/ip
curl -x http://localhost:8080 http://httpbin.org/user-agent
```

#### Test 2: HTTPS Tunneling

```bash
curl -x http://localhost:8080 https://httpbin.org/ip
curl -x http://localhost:8080 https://www.google.com
```

#### Test 3: Large Transfers

```bash
# Download large file through proxy
curl -x http://localhost:8080 \
    -O http://releases.ubuntu.com/20.04/ubuntu-20.04-live-server-amd64.iso
```

#### Test 4: Concurrent Connections

```bash
# Install GNU Parallel
sudo apt-get install parallel

# Test 10 concurrent requests
parallel -j 10 "curl -x http://localhost:8080 http://example.com" \
    ::: {1..10}
```

### Automated Testing

#### cppcheck - Static Analysis

```bash
# Basic check
cppcheck src/

# Detailed check with all warnings
cppcheck --enable=all --verbose src/

# Generate report
cppcheck --xml src/ 2> report.xml
```

#### Valgrind - Memory Check

```bash
# Build with debug symbols
make DEBUG=1

# Run with valgrind
valgrind --leak-check=full \
         --show-leak-kinds=all \
         --track-origins=yes \
         --verbose \
         ./proxy-server -p 8080 &

# Run some requests
sleep 2
curl -x http://localhost:8080 http://example.com
sleep 2

# Terminate
pkill proxy-server
```

#### Helgrind - Thread Check

```bash
# For multi-threaded builds
valgrind --tool=helgrind ./proxy-server -p 8080
```

## Debugging

### GDB - GNU Debugger

#### Basic Usage

```bash
# Build with debug symbols
make DEBUG=1

# Start debugging
gdb ./proxy-server

# In GDB:
(gdb) run -p 8080
(gdb) break request_parse
(gdb) continue
(gdb) print url
(gdb) next
(gdb) quit
```

#### Common Commands

```gdb
# Breakpoints
break function_name
break file.c:42
break request.c:parse_request

# Execution
run [args]
continue
next
step
finish

# Inspection
print variable
print *pointer
print array[0..10]
backtrace
info locals
info args

# Debugging assistance
list
disassemble
x/10i $pc  # Show 10 instructions
```

#### Setting Breakpoints

```bash
# Terminal 1: Start GDB with break on main
gdb -ex "break main" -ex "run -p 8080" ./proxy-server

# Terminal 2: Send request
curl -x http://localhost:8080 http://example.com

# Back in GDB: Inspect and debug
(gdb) backtrace
(gdb) print request
(gdb) continue
```

### LLDB - LLVM Debugger (macOS)

```bash
# Start debugging
lldb ./proxy-server

(lldb) breakpoint set --name request_parse
(lldb) run -p 8080
(lldb) continue
(lldb) frame variable
```

## Performance Analysis

### Profiling with gprof

```bash
# Compile with profiling
gcc -pg -o proxy-server *.c -lm

# Run with typical workload
./proxy-server -p 8080 &
PROXY_PID=$!

# Generate load
for i in {1..1000}; do
    curl -s -x http://localhost:8080 http://example.com > /dev/null &
done

sleep 10
kill $PROXY_PID

# Generate profile report
gprof proxy-server gmon.out > profile.txt
cat profile.txt
```

### Profiling with perf

```bash
# Install perf
sudo apt-get install linux-tools-generic

# Run with perf
sudo perf record -p $(pgrep proxy-server) -- sleep 10

# Generate report
sudo perf report
```

### Benchmark with ab (Apache Bench)

```bash
# Simple benchmark
ab -n 1000 -c 10 -X localhost:8080 http://httpbin.org/delay/1

# Output analysis
ab -n 10000 -c 50 -g results.tsv -X localhost:8080 http://example.com

# Plot results (requires gnuplot)
gnuplot -e "set terminal png; set output 'results.png'; plot 'results.tsv' using 5 title 'Response Time' with lines"
```

### Benchmark with wrk

```bash
# Install wrk
git clone https://github.com/wg/wrk.git
cd wrk && make

# Run benchmark
./wrk -t4 -c100 -d30s -x localhost:8080 http://example.com

# Lua scripting for custom requests
cat > req.lua << EOF
request = function()
    return wrk.format(nil, "/example")
end
EOF

./wrk -t4 -c100 -d30s -s req.lua -x localhost:8080 http://example.com
```

## Troubleshooting

### Build Issues

#### Error: `gcc: command not found`

```bash
# Install GCC
sudo apt-get install gcc build-essential

# Or verify path
which gcc
```

#### Error: `Makefile not found`

```bash
# Ensure you're in the right directory
pwd
ls -la

# Verify Makefile exists
cat Makefile
```

#### Error: `undefined reference to 'socket'`

```bash
# Link with socket library
gcc -o proxy-server *.c -lsocket -lnsl
```

### Runtime Issues

#### Error: `Address already in use`

```bash
# Find process using port
lsof -i :8080
netstat -tulpn | grep 8080

# Kill the process
kill -9 <PID>

# Or use different port
./proxy-server -p 9090
```

#### Error: `Permission denied` (port < 1024)

```bash
# Use sudo
sudo ./proxy-server -p 80

# Or use port > 1024
./proxy-server -p 8080

# Or set capabilities
sudo setcap cap_net_bind_service=+ep ./proxy-server
```

#### Segmentation fault

```bash
# Run with GDB
gdb ./proxy-server
(gdb) run -p 8080
# Triggers seg fault
(gdb) backtrace
# Shows where it crashed
```

## Contributing Changes

1. Create feature branch
2. Make changes
3. Build and test
4. Submit pull request

```bash
git checkout -b feature/my-feature
# ... make changes ...
make clean && make
make test
git commit -m "Add my feature"
git push origin feature/my-feature
# Open PR on GitHub
```

---

**Last Updated**: 2026-04-26
