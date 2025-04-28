# Readers-Writers Problem Implementation

## Overview
This project implements the classic Readers-Writers synchronization problem using C++, demonstrating six different approaches with various prioritization schemes and synchronization mechanisms.

## The Problem
The Readers-Writers problem involves managing access to a shared resource where:
- Multiple readers can access the resource simultaneously without conflict
- Writers need exclusive access (no other readers or writers can access the resource while a writer is active)
- The solution must prevent deadlocks, starvation, and race conditions

## Solution Approaches

| Implementation | File | Prioritization | Synchronization Mechanism |
|----------------|------|----------------|--------------------------|
| Writers-Priority | `readers_writers.cpp` | Writers > Readers | mutex + condition variable |
| Semaphore-based | `readers_writers_semaphore.cpp` | Writers > Readers | POSIX semaphores |
| Readers-Priority | `readers_writers_readers_priority.cpp` | Readers > Writers | mutex + condition variable |
| Fair/Starvation-Free | `readers_writers_fair.cpp` | FIFO order | Request queue + mutex |
| std::shared_mutex | `readers_writers_shared_mutex.cpp` | Implementation dependent | C++17 shared_mutex |
| Monitor-based | `readers_writers_monitor.cpp` | Configurable | Monitor pattern |
| Educational | `readers_writers_educational.cpp` | Writers > Readers | Heavily commented version |

## Building and Running

You can use the provided Makefile:

```bash
# Build all implementations
make

# Run specific implementations
make run_writers_priority
make run_semaphore
make run_readers_priority
make run_fair
make run_shared_mutex
make run_monitor

# Run all implementations in sequence
make run_all

# Run benchmarks on all implementations
make benchmark
```

Or run the unified demonstration script:

```bash
# Run all demos in sequence
./readers_writers_demo.sh
```

You can also set the number of readers, writers, and operations:

```bash
# Custom configuration
READERS=15 WRITERS=5 OPERATIONS=3 ./readers_writers_fair
```

## Demonstration and Testing

### Comprehensive Demo Script

We've created a single script that demonstrates all implementations:

```bash
./readers_writers_demo.sh
```

This script:
1. Compiles all implementations
2. Runs each implementation with a configurable time limit
3. Shows real-time statistics
4. Displays colorful output to differentiate implementations
5. Supports custom parameters via environment variables

### Testing Performance

To compare performance across implementations:

```bash
# Run with default settings
./readers_writers_demo.sh --benchmark

# Run with custom settings
READERS=20 WRITERS=10 OPERATIONS=5 ./readers_writers_demo.sh --benchmark
```

The benchmark collects and displays:
- Total reads and writes completed
- Average wait times for both reader and writer threads
- Thread-to-thread fairness metrics
- Resource utilization statistics

## Implementation Details

### Key Features

All implementations focus on addressing key concurrency challenges:
- **Deadlock Prevention**: Careful lock ordering and acquisition
- **Starvation Handling**: Various prioritization schemes
- **Race Condition Prevention**: Protected critical sections
- **Performance Optimization**: Efficient synchronization

### Source Code Organization

- **readers_writers.cpp**: Writers-priority implementation
- **readers_writers_readers_priority.cpp**: Readers-priority implementation
- **readers_writers_fair.cpp**: Fair/queue-based implementation
- **readers_writers_semaphore.cpp**: Semaphore-based implementation
- **readers_writers_shared_mutex.cpp**: C++17 shared_mutex implementation
- **readers_writers_monitor.cpp**: Monitor-based implementation
- **readers_writers_educational.cpp**: Extensively commented educational version

## Documentation

- **[Report.md](Report.md)**: Comprehensive project report with detailed analysis of all implementations
- **[README.md](README.md)**: Installation and usage guide


