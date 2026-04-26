# Proxy Server

A lightweight HTTP/HTTPS proxy server implementation written in C. This proxy server forwards client requests to remote servers and returns responses, enabling features like request filtering, caching, and traffic monitoring.

## 📋 Table of Contents

- [Features](#features)
- [Project Structure](#project-structure)
- [Prerequisites](#prerequisites)
- [Building](#building)
- [Installation](#installation)
- [Usage](#usage)
- [Configuration](#configuration)
- [Examples](#examples)
- [Architecture](#architecture)
- [Performance Considerations](#performance-considerations)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

## ✨ Features

- **HTTP/HTTPS Proxying**: Forward HTTP and HTTPS requests to remote servers
- **Lightweight**: Written in pure C with minimal dependencies
- **Concurrent Connections**: Handle multiple client connections efficiently
- **Request Forwarding**: Transparently proxy client requests with header preservation
- **Response Caching** (if implemented): Optional caching of responses
- **Customizable**: Easy to extend and modify for specific use cases
- **Cross-Platform**: Compatible with Linux, macOS, and other Unix-like systems

## 📁 Project Structure

```
proxy-Server-/
├── README.md              # This file
├── Makefile              # Build configuration
├── src/                  # Source code directory
│   ├── proxy.c          # Main proxy implementation
│   ├── server.c         # Server socket handling
│   ├── client.c         # Client request handling
│   ├── request.c        # HTTP request parsing
│   ├── response.c       # HTTP response handling
│   └── utils.c          # Utility functions
├── include/             # Header files
│   ├── proxy.h
│   ├── server.h
│   ├── client.h
│   ├── request.h
│   ├── response.h
│   └── utils.h
├── build/               # Compiled binaries (generated)
└── docs/                # Documentation (optional)
```

## 🔧 Prerequisites

Before building the proxy server, ensure you have:

- **GCC** or **Clang** compiler
- **Make** build tool
- **Linux** or **Unix-like** operating system
- Standard C library (libc)
- **Root access** (for binding to ports below 1024)

### Ubuntu/Debian:
```bash
sudo apt-get install build-essential gcc make
```

### macOS (using Homebrew):
```bash
brew install gcc make
```

### CentOS/RHEL:
```bash
sudo yum install gcc make glibc-devel
```

## 🏗️ Building

### Basic Build

```bash
make
```

### Clean Build (Remove old binaries)

```bash
make clean
```

### Rebuild Everything

```bash
make clean && make
```

### Custom Compilation Flags

```bash
make CFLAGS="-O2 -Wall -std=c99"
```

## 📦 Installation

### Local Installation

```bash
# Build the project
make

# Run directly
./proxy-server -p 8080
```

### System-Wide Installation

```bash
# Build the project
make

# Install to /usr/local/bin
sudo make install

# Run from anywhere
proxy-server -p 8080
```

## 🚀 Usage

### Basic Usage

```bash
# Start proxy server on default port
./proxy-server

# Start proxy server on specific port
./proxy-server -p 8080

# Start with verbose output
./proxy-server -v

# Show help
./proxy-server -h
```

### Command-Line Options

| Option | Description | Example |
|--------|-------------|---------|
| `-p, --port` | Port to listen on (default: 8888) | `-p 3128` |
| `-h, --help` | Display help message | `-h` |
| `-v, --verbose` | Enable verbose logging | `-v` |
| `-c, --config` | Configuration file path | `-c /etc/proxy.conf` |
| `-t, --timeout` | Connection timeout in seconds | `-t 30` |
| `-b, --bind` | Bind to specific IP address | `-b 127.0.0.1` |

## ⚙️ Configuration

### Configuration File Format

Create a configuration file (e.g., `proxy.conf`):

```conf
# Proxy Server Configuration

# Server settings
port=8888
bind_address=0.0.0.0
timeout=30

# Logging
verbose=true
log_file=/var/log/proxy.log

# Performance
max_connections=1024
buffer_size=8192

# Filtering (optional)
allow_https=true
deny_ips=192.168.1.100,10.0.0.5
```

### Using Configuration File

```bash
./proxy-server -c proxy.conf
```

## 📝 Examples

### Example 1: Basic HTTP Proxying

```bash
# Start proxy on port 8080
./proxy-server -p 8080

# In another terminal, test with curl
curl -x http://localhost:8080 http://www.example.com
```

### Example 2: Proxying HTTPS Requests

```bash
# Request through proxy
curl -x http://localhost:8080 https://www.cs.princeton.edu/

# Or using the URL format
curl http://localhost:8080/https://www.cs.princeton.edu/
```

### Example 3: Custom Configuration

```bash
# Create custom config
cat > my_proxy.conf << EOF
port=3128
verbose=true
timeout=60
EOF

# Run with config
./proxy-server -c my_proxy.conf
```

### Example 4: Request Through Proxy from Browser

```
Proxy URL: http://localhost:8080/https://www.cs.princeton.edu/
```

Configure your browser/application to use `http://localhost:8080` as HTTP proxy.

## 🏗️ Architecture

### Request Flow

```
Client Request
    ↓
[Proxy Server] ← Accepts connection on listening port
    ↓
[Request Parser] ← Parses HTTP headers and request line
    ↓
[Remote Connection] ← Establishes connection to target server
    ↓
[Forward Request] ← Sends request to remote server
    ↓
[Receive Response] ← Receives response from remote server
    ↓
[Send to Client] ← Forwards response to client
    ↓
[Close Connection] ← Closes connections
```

### Key Components

1. **Server Socket**: Listens for incoming client connections
2. **Request Parser**: Parses HTTP requests and extracts target URL
3. **Remote Connector**: Establishes connections to remote servers
4. **Request Forwarder**: Forwards client requests to remote servers
5. **Response Handler**: Receives and forwards responses back to clients

## ⚡ Performance Considerations

### Optimization Tips

1. **Increase Buffer Size**: Modify `BUFFER_SIZE` in the source for better throughput
   ```c
   #define BUFFER_SIZE 65536  // 64KB instead of default
   ```

2. **Increase Max Connections**: Adjust file descriptor limits
   ```bash
   ulimit -n 65536
   ```

3. **Use Connection Pooling**: Reuse connections to frequently accessed servers

4. **Enable Caching**: Implement response caching for frequently accessed resources

5. **Compile Optimizations**:
   ```bash
   make CFLAGS="-O3 -march=native -flto"
   ```

### Benchmarking

```bash
# Test with Apache Bench
ab -n 1000 -c 10 -X localhost:8080 http://www.example.com

# Test with wrk
wrk -t4 -c100 -d30s -x localhost:8080 http://www.example.com
```

## 🐛 Troubleshooting

### Issue: "Port already in use"

```bash
# Find process using the port
lsof -i :8080
# or
netstat -tulpn | grep 8080

# Kill the process
kill -9 <PID>
```

### Issue: "Permission denied" (binding to port < 1024)

```bash
# Use sudo for privileged ports
sudo ./proxy-server -p 80

# Or use a port > 1024
./proxy-server -p 8080
```

### Issue: "Connection refused" from remote server

- Check internet connectivity
- Verify target server is accessible
- Check firewall rules
- Examine proxy logs for detailed errors

### Issue: HTTPS requests not working

- Ensure HTTP CONNECT method is supported
- Check SSL/TLS certificate validation (if required)
- Verify remote server accepts HTTPS connections

### Enable Debug Mode

```bash
# Compile with debug symbols
make CFLAGS="-g -O0 -DDEBUG"

# Run with GDB debugger
gdb ./proxy-server
(gdb) run -p 8080
```

## 🤝 Contributing

Contributions are welcome! To contribute:

1. **Fork** the repository
2. **Create** a feature branch (`git checkout -b feature/amazing-feature`)
3. **Commit** your changes (`git commit -m 'Add amazing feature'`)
4. **Push** to the branch (`git push origin feature/amazing-feature`)
5. **Open** a Pull Request

### Coding Standards

- Follow Linux kernel coding style
- Use meaningful variable names
- Add comments for complex logic
- Test your changes thoroughly
- Update documentation

## 📋 License

This project is currently unlicensed. To add a license:

1. Choose a license from [choosealicense.com](https://choosealicense.com/)
2. Add `LICENSE` file to repository
3. Update this section accordingly

**Recommended licenses for C projects:**
- **MIT**: Simple and permissive
- **GPL-3.0**: Copyleft open-source
- **Apache-2.0**: Professional open-source

## 📞 Support & Contact

- **Issues**: [Report bugs](https://github.com/M-Taha-Raza12/proxy-Server-/issues)
- **Discussions**: [Start a discussion](https://github.com/M-Taha-Raza12/proxy-Server-/discussions)
- **Author**: [M-Taha-Raza12](https://github.com/M-Taha-Raza12)

## 📚 Additional Resources

- [HTTP/1.1 Specification (RFC 7230-7235)](https://tools.ietf.org/html/rfc7230)
- [Socket Programming in C](https://beej.us/guide/bgnet/)
- [Linux man pages: socket(2), connect(2), listen(2)](https://man7.org/linux/man-pages/)
- [GCC Compiler Flags Reference](https://gcc.gnu.org/onlinedocs/gcc/Invoking-GCC.html)

---

**Last Updated**: 2026-04-26  
**Repository**: [M-Taha-Raza12/proxy-Server-](https://github.com/M-Taha-Raza12/proxy-Server-)
