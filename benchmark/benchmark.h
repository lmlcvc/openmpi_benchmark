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
#include <functional>
#include <string>

#include "../communication/communication_interface.h"
#include "../unit/unit.h"

struct ArgumentEntry
{
    char option;
    std::string value;
};

enum CommunicationType
{
    COMM_UNDEFINED,
    COMM_SCAN,
    COMM_FIXED_BLOCKING,
    COMM_FIXED_NONBLOCKING,
    COMM_VARIABLE_BLOCKING,
    COMM_VARIABLE_NONBLOCKING
};

class Benchmark : public CommunicationInterface
{
public:
    virtual ~Benchmark() {}
    virtual void run() = 0;
    virtual void performWarmup() = 0;

protected:
    timespec diff(timespec start, timespec end);
    std::vector<std::pair<int, int>> findSubarrayIndices(std::size_t messageSize);
    std::pair<double, double> calculateThroughput(timespec startTime, timespec endTime, std::size_t bytesTransferred, std::size_t iterations);

    virtual void warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int ruRank, int buRank) = 0;
    virtual void parseArguments(std::vector<ArgumentEntry> args) = 0;

    int m_rank;

    std::size_t m_iterations = 1e5;       // communication steps to be printed
    std::size_t m_warmupIterations = 100; // iteration count for warmup-related throughput calculation
    std::size_t m_syncIterations = 1e4;

    const std::size_t m_minIterations = 1e5;

    typedef std::unique_ptr<void, std::function<void(void *)>> buffer_t;
};

#endif // BENCHMARK_H
