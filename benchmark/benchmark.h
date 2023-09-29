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

    const std::string getPhasesFilepath() { return m_phasesFilepath; }
    void setPhasesFilepath(std::string path) { m_phasesFilepath = path; }

    const std::string getAvgThroughputFilepath() { return m_avgThroughputFilepath; }
    void setAvgThroughputFilepath(std::string path) { m_avgThroughputFilepath = path; }

protected:
    timespec diff(timespec start, timespec end);
    std::vector<std::pair<int, int>> findSubarrayIndices(std::size_t bufferSize);
    std::pair<double, double> calculateThroughput(timespec startTime, timespec endTime, std::size_t bytesTransferred, std::size_t iterations);

    virtual void warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int ruRank, int buRank) = 0;
    virtual void parseArguments(std::vector<ArgumentEntry> args) = 0;

    int m_rank;
    std::string m_hostname;

    std::size_t m_iterations = 1e4;       // communication steps to be printed
    std::size_t m_warmupIterations = 100; // iteration count for warmup-related throughput calculation

    const std::size_t m_minIterations = 1e4;

    typedef std::unique_ptr<void, std::function<void(void *)>> buffer_t;

    std::string m_phasesFilepath;
    std::string m_avgThroughputFilepath;
};

#endif // BENCHMARK_H
