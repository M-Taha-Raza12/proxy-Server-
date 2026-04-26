# Proxy Server Documentation

## Overview

The Proxy Server is a lightweight, high-performance HTTP/HTTPS proxy implementation written in C. It forwards client requests to remote servers and handles responses efficiently, making it suitable for network filtering, content caching, and traffic analysis applications.

## Quick Start

### Installation

```bash
# Clone the repository
git clone https://github.com/M-Taha-Raza12/proxy-Server-.git
cd proxy-Server-

# Build
make

# Run
./proxy-server -p 8080
```

### First Request

```bash
# In another terminal
curl -x http://localhost:8080 http://www.example.com
```

## Table of Contents

1. [Architecture](#architecture)
2. [API Reference](#api-reference)
3. [Configuration](#configuration)
4. [Examples](#examples)
5. [Troubleshooting](#troubleshooting)
6. [Performance Tuning](#performance-tuning)

## Architecture

### Overview

```
┌─────────────┐
│   Clients   │
└──────┬──────┘
       │ HTTP/HTTPS
       ▼
┌──────────────────┐
│  Proxy Server    │
│  - Listen on     │
│    specified     │
│    port          │
└────────┬─────────┘
         │ Parse Request
         ▼
┌──────────────────┐
│ Request Parser   │
│  - Extract URL   │
│  - Parse Headers │
└────────┬─────────┘
         │ Connect
         ▼
┌──────────────────┐
│ Remote Server    │
│  Connection      │
└────────┬─────────┘
         │ Response
         ▼
┌──────────────────┐
│ Response Handler │
│  - Forward Data  │
│  - Manage Buffer │
└────────┬─────────┘
         │ Send Response
         ▼
┌─────────────┐
│   Clients   │
└─────────────┘
```

### Key Components

#### 1. Main Server Loop
- Listens on configured port
- Accepts incoming connections
- Creates handlers for each client

#### 2. Request Parser
- Extracts HTTP method, path, headers
- Validates request format
- Determines target server

#### 3. Connection Manager
- Establishes connections to remote servers
- Manages connection state
- Handles timeouts and retries

#### 4. Data Forwarding
- Buffers data efficiently
- Handles partial transfers
- Maintains connection state

#### 5. Response Handler
- Receives remote server response
- Forwards to client
- Handles chunked encoding if needed

## API Reference

### Command-Line Interface

```bash
./proxy-server [OPTIONS]
```

#### Options

| Option | Argument | Default | Description |
|--------|----------|---------|-------------|
| `-p, --port` | PORT | 8888 | Listen port |
| `-b, --bind` | ADDRESS | 0.0.0.0 | Bind address |
| `-t, --timeout` | SECONDS | 30 | Connection timeout |
| `-v, --verbose` | - | false | Verbose logging |
| `-c, --config` | FILE | - | Config file |
| `-h, --help` | - | - | Show help |

### Configuration File Format

```ini
# Network Configuration
port=8888
bind_address=0.0.0.0
timeout=30

# Logging
verbose=true
log_level=info
log_file=/var/log/proxy.log

# Performance
max_connections=1024
buffer_size=8192

# Features
enable_caching=false
cache_ttl=3600
```

### Environment Variables

```bash
# Override configuration
PROXY_PORT=9000
PROXY_BIND_ADDRESS=127.0.0.1
PROXY_TIMEOUT=60
PROXY_VERBOSE=1
```

## Configuration

### Basic Configuration

**File: `proxy.conf`**
```ini
port=3128
bind_address=0.0.0.0
timeout=30
verbose=false
```

**Usage:**
```bash
./proxy-server -c proxy.conf
```

### Advanced Configuration

**File: `advanced.conf`**
```ini
# Server
port=3128
bind_address=0.0.0.0
timeout=30

# Performance
max_connections=2048
buffer_size=65536
max_request_size=1048576

# Logging
verbose=true
log_file=/var/log/proxy.log
log_level=debug

# Filtering
enable_filter=true
deny_domains=private.example.com
allow_domains=*.example.com

# Caching
enable_caching=true
cache_ttl=3600
max_cache_size=1073741824
```

### Environment-Based Configuration

```bash
#!/bin/bash

export PROXY_PORT=${PORT:-8080}
export PROXY_BIND_ADDRESS=${BIND_ADDR:-0.0.0.0}
export PROXY_TIMEOUT=${TIMEOUT:-30}
export PROXY_VERBOSE=${VERBOSE:-false}

./proxy-server
```

## Examples

### Example 1: Basic HTTP Proxy

```bash
# Start proxy
./proxy-server -p 8080

# Test with curl
curl -x http://localhost:8080 http://httpbin.org/ip

# Test with wget
wget -e use_proxy=yes -e http_proxy=localhost:8080 http://example.com
```

### Example 2: HTTPS Proxying

```bash
# Request HTTPS through proxy
curl -x http://localhost:8080 https://www.example.com

# Browser configuration
# HTTP Proxy: localhost
# Port: 8080
```

### Example 3: Docker Deployment

**Dockerfile:**
```dockerfile
FROM ubuntu:20.04

RUN apt-get update && apt-get install -y \
    build-essential \
    gcc \
    make

COPY . /app
WORKDIR /app

RUN make

EXPOSE 8080

CMD ["./proxy-server", "-p", "8080", "-b", "0.0.0.0"]
```

**Build and run:**
```bash
docker build -t proxy-server .
docker run -p 8080:8080 proxy-server
```

### Example 4: Monitoring and Logging

```bash
# Run with verbose logging
./proxy-server -v -p 8080 2>&1 | tee proxy.log

# Monitor in real-time
tail -f proxy.log | grep "ERROR\|WARN"

# Connection monitoring
netstat -an | grep 8080 | wc -l
```

### Example 5: Load Testing

```bash
# Install Apache Bench (if not present)
sudo apt-get install apache2-utils

# Simple load test
ab -n 1000 -c 10 -X localhost:8080 http://example.com

# Detailed results
ab -n 1000 -c 10 -X localhost:8080 -g results.tsv http://example.com
```

## Troubleshooting

### Common Issues

#### Port Already in Use

```bash
# Find process using port
lsof -i :8080

# Alternative
netstat -tulpn | grep 8080

# Kill process
kill -9 <PID>

# Or use different port
./proxy-server -p 9090
```

#### Permission Denied (Privileged Port)

```bash
# Use sudo for ports < 1024
sudo ./proxy-server -p 80

# Or use unprivileged port
./proxy-server -p 8080

# Or configure iptables redirect
sudo iptables -t nat -A PREROUTING -p tcp --dport 80 -j REDIRECT --to-port 8080
```

#### Connection Refused

```bash
# Check if proxy is running
ps aux | grep proxy-server

# Check port binding
netstat -an | grep 8080

# Verify firewall
sudo ufw allow 8080
```

#### Remote Server Connection Failed

```bash
# Test network connectivity
ping www.example.com

# Check DNS resolution
nslookup www.example.com

# Trace connection attempt
curl -v -x http://localhost:8080 http://example.com

# Check with netstat
netstat -tuna | grep ESTABLISHED
```

### Debug Mode

```bash
# Compile with debug symbols
make CFLAGS="-g -O0 -DDEBUG"

# Run under GDB
gdb ./proxy-server

(gdb) run -p 8080 -v
(gdb) break request_parse
(gdb) continue
(gdb) print url
(gdb) step
```

### Memory Issues

```bash
# Check for memory leaks
valgrind --leak-check=full --show-leak-kinds=all ./proxy-server -p 8080 &

# Let it run for a bit, then:
kill -9 <PID>

# View report
# Valgrind will show leaked blocks
```

## Performance Tuning

### System Configuration

```bash
# Increase file descriptor limit
ulimit -n 65536

# Increase network backlog
sudo sysctl -w net.core.somaxconn=4096

# Increase TCP connection tracking
sudo sysctl -w net.ipv4.tcp_max_syn_backlog=8192
```

### Proxy Configuration

```ini
# Larger buffer for high throughput
buffer_size=65536

# More connections
max_connections=4096

# Timeout for slow clients
timeout=60
```

### Compilation Optimization

```bash
# Maximum optimization
make CFLAGS="-O3 -march=native -flto -ffast-math"

# Profile-guided optimization
gcc -fprofile-generate -O3 -o proxy-server *.c
# Run proxy with typical workload...
gcc -fprofile-use -O3 -o proxy-server *.c
```

### Benchmarking

```bash
# Sustained load test
ab -t 60 -c 100 -X localhost:8080 http://example.com

# Measure throughput
ab -n 10000 -c 100 -X localhost:8080 http://example.com

# Concurrent connections
siege -c 100 -r 100 -X localhost:8080 http://example.com
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for guidelines.

## License

See LICENSE file for details.

## Support

- **Issues**: [GitHub Issues](https://github.com/M-Taha-Raza12/proxy-Server-/issues)
- **Discussions**: [GitHub Discussions](https://github.com/M-Taha-Raza12/proxy-Server-/discussions)

---

**Last Updated**: 2026-04-26  
**Repository**: [M-Taha-Raza12/proxy-Server-](https://github.com/M-Taha-Raza12/proxy-Server-)
