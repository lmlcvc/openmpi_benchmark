#ifndef SCANBENCHMARK_H
#define SCANBENCHMARK_H

#include "benchmark.h"

class ScanBenchmark : public Benchmark
{
public:
    ScanBenchmark(std::vector<ArgumentEntry> args);
    virtual ~ScanBenchmark() override {}

    void run() override;

    void performWarmup() override;
    void warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int ruRank, int buRank) override;

private:
    void allocateMemory();
    void parseArguments(std::vector<ArgumentEntry> args) override;
    void printRunInfo(std::size_t messageSize, double throughput);

    std::size_t m_maxPower = 22;

    std::size_t m_sndBufferSize = 10; // circular buffer sizes (in messages)
    std::size_t m_rcvBufferSize = 10;

    buffer_t m_memSndPtr;
    buffer_t m_memRcvPtr;

    std::size_t m_sndBufferBytes = 1e7;
    std::size_t m_rcvBufferBytes = 1e7;

    int8_t *m_bufferSnd;
    int8_t *m_bufferRcv;
};

#endif // SCANBENCHMARK_H