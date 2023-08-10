#ifndef CONTINUOUSBENCHMARK_H
#define CONTINUOUSBENCHMARK_H

#include "benchmark.h"

class ContinuousBenchmark : public Benchmark
{
public:
    ContinuousBenchmark(int argc, char **argv, int rank);
    virtual ~ContinuousBenchmark() override {}
    void setup(int rank) override;
    void run() override;

private:
    const std::size_t minMessageSize = 1e4;

    std::size_t m_messageSize;
};

#endif // CONTINUOUSBENCHMARK_H