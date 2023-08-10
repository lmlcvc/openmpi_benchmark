#ifndef BENCHMARK_H
#define BENCHMARK_H

#include <iostream>
#include <iomanip>
#include <limits>
#include <cstdlib>
#include <cstdint>
#include <cstddef>
#include <memory>
#include <unistd.h>
#include <time.h>
#include <mpi.h>
#include <chrono>
#include <thread>
#include <cmath>
#include <csignal>
#include <vector>
#include <cstring>
#include <algorithm>

class Benchmark
{
public:
    Benchmark(int argc, char **argv) {}  // TODO: abstract
    virtual ~Benchmark() {}
    virtual void setup() = 0;
    virtual void run() = 0;

protected:
    timespec diff(timespec start, timespec end);
    virtual void allocateMemory();

    std::size_t m_iterations = 1e4;       // communication steps to be printed
    std::size_t m_bufferSize = 10;        // circular buffer size (in chunk count)
    std::size_t m_warmupIterations = 100; // iteration count for warmup-related throughput calculation

    const std::size_t minIterations = 1e5;

    const long pageSize = sysconf(_SC_PAGESIZE);
    void *mem_snd = nullptr;
    void *mem_rcv = nullptr;
    std::size_t m_alignSize;

    // std::unique_ptr<void, decltype(&free)> memoryPtr;
};

#endif // BENCHMARK_H
