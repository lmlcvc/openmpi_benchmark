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

#include "../communication/communication_interface.h"
#include "../unit/readout_unit.h"
#include "../unit/builder_unit.h"

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

    void performWarmup();

protected:
    timespec diff(timespec start, timespec end);
    std::vector<std::pair<int, int>> findSubarrayIndices(std::size_t messageSize);
    std::pair<double, double> calculateThroughput(timespec startTime, timespec endTime, std::size_t bytesTransferred, std::size_t iterations);

    void allocateMemory();
    virtual void parseArguments(std::vector<ArgumentEntry> args) = 0;

    virtual void warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int8_t rank);

    int m_rank;
    int m_nodesCount;
    std::size_t m_currentPhase = 0;

    std::size_t m_iterations = 1e5;       // communication steps to be printed
    std::size_t m_warmupIterations = 100; // iteration count for warmup-related throughput calculation
    std::size_t m_syncIterations = 1e4;

    const std::size_t m_minIterations = 1e5;

    typedef std::unique_ptr<void, std::function<void(void *)>> buffer_t;

    buffer_t m_memSndPtr;
    buffer_t m_memRcvPtr;

    std::size_t m_sndBufferBytes = 1e7;
    std::size_t m_rcvBufferBytes = 1e7;

    int8_t *m_bufferSnd;
    int8_t *m_bufferRcv;

    std::unique_ptr<ReadoutUnit> m_readoutUnit;
    std::unique_ptr<BuilderUnit> m_builderUnit;
};

#endif // BENCHMARK_H
