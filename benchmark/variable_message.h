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
    void parseArguments(std::vector<ArgumentEntry> args) override;

    std::size_t m_messageSizeVariants = 100;
    std::vector<std::size_t> m_messageSizes;
};

#endif // BENCHMARKVARIABLEMESSAGE_H
