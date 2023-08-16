#ifndef BENCHMARKVARIABLEMESSAGE_H
#define BENCHMARKVARIABLEMESSAGE_H

#include <random>

#include "continuous_benchmark.h"

class BenchmarkVariableMessage : public ContinuousBenchmark
{
public:
    BenchmarkVariableMessage(std::vector<ArgumentEntry> args, int rank);
    virtual ~BenchmarkVariableMessage() override {}

    void run() override;

private:
    void initMessageSizes();

    std::size_t m_maxMessageSize = 1e7;
    std::vector<std::size_t> m_messageSizes;
};

#endif // BENCHMARKVARIABLEMESSAGE_H
