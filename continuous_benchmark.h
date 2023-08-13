#ifndef CONTINUOUSBENCHMARK_H
#define CONTINUOUSBENCHMARK_H

#include "benchmark.h"

class ContinuousBenchmark : public Benchmark
{
public:
    ContinuousBenchmark(int argc, char **argv, int rank);
    virtual ~ContinuousBenchmark() override {}
    void setup() override;
    void run() override;

private:
    void performWarmup() override;

    const std::size_t minMessageSize = 1e4;

    std::size_t m_messageSize;              // message size in bytes
};

#endif // CONTINUOUSBENCHMARK_H