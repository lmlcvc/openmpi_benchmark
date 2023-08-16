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

#include "../communication/communication_interface.h"

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

// TODO: handle recieve buffer and message size relation
// (cannot make the recieved stuff exceed buffer)
// either handle sizes or recieve in loop

// XXX: look into loop recieve - same for send
class Benchmark : public CommunicationInterface
{
public:
    virtual ~Benchmark() {}
    virtual void run() = 0;

protected:
    void allocateMemory();
    std::vector<std::pair<int, int>> findSubarrayIndices(std::size_t messageSize);
    std::pair<double, double> calculateThroughput(timespec startTime, timespec endTime, std::size_t bytesTransferred, std::size_t iterations);
    void printRunInfo(double rtt, double throughput, int errorMessagesCount); // TODO: remove from here and reuse

    virtual void parseArguments(std::vector<ArgumentEntry> args) = 0;

    virtual void warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int8_t rank);
    void performWarmup();

    int m_rank;

    std::size_t m_iterations = 1e5;       // communication steps to be printed
    std::size_t m_warmupIterations = 100; // iteration count for warmup-related throughput calculation

    const std::size_t m_minIterations = 1e5;

    void *memSnd = nullptr;
    void *memRcv = nullptr;

    std::size_t m_sndBufferBytes = 1e7;
    std::size_t m_rcvBufferBytes = 1e7;

    int8_t *m_bufferSnd;
    int8_t *m_bufferRcv;
};

#endif // BENCHMARK_H
