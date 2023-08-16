#ifndef SCANBENCHMARK_H
#define SCANBENCHMARK_H

#include "benchmark.h"

class ScanBenchmark : public Benchmark
{
public:
    ScanBenchmark(std::vector<ArgumentEntry> args, int rank);
    virtual ~ScanBenchmark() override {}

    void run() override;

private:
    void parseArguments(std::vector<ArgumentEntry> args) override;
    void printRunInfo(std::size_t messageSize, double throughput);

    std::size_t m_maxPower = 22;

    std::size_t m_sndBufferSize = 10; // circular buffer sizes (in messages)
    std::size_t m_rcvBufferSize = 10;
};

#endif // SCANBENCHMARK_H