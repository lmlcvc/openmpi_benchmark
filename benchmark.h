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
    virtual ~Benchmark() {}
    virtual void setup() = 0;
    virtual void run() = 0;

protected:
    void allocateMemory();
    std::vector<std::pair<int, int>> findSubarrayIndices(std::size_t messageSize);
    std::pair<double, double> calculateThroughput(timespec startTime, timespec endTime, std::size_t messageSize, std::size_t iterations);

    virtual std::size_t rtCommunication(std::size_t sndBufferSize, std::size_t rcvBufferSize, std::size_t messageSize, std::size_t interval);
    virtual void warmupCommunication(int8_t *bufferSnd, std::vector<std::pair<int, int>> subarrayIndices, int8_t rank);
    virtual void performWarmup() = 0;

    int m_rank;

    std::size_t m_iterations;    // communication steps to be printed
    std::size_t m_sndBufferSize; // circular buffer sizes (in messages)
    std::size_t m_rcvBufferSize;
    std::size_t m_warmupIterations; // iteration count for warmup-related throughput calculation

    const std::size_t minIterations = 1e5;

    
    void *memSnd;
    void *memRcv;
    std::size_t m_alignSize;

    int8_t *m_bufferSnd;
    int8_t *m_bufferRcv;

    // std::unique_ptr<void, decltype(&free)> memoryPtr;
};

#endif // BENCHMARK_H
