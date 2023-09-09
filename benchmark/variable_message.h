#ifndef BENCHMARKVARIABLEMESSAGE_H
#define BENCHMARKVARIABLEMESSAGE_H

#include <random>

#include "continuous_benchmark.h"

class BenchmarkVariableMessage : public ContinuousBenchmark
{
public:
    BenchmarkVariableMessage(std::vector<ArgumentEntry> args, int rank, int size, CommunicationType commType);
    virtual ~BenchmarkVariableMessage() override {}

private:
    void initMessageSizes();
    void parseArguments(std::vector<ArgumentEntry> args) override;

    std::size_t m_messageSizeVariants = 100;
};

#endif // BENCHMARKVARIABLEMESSAGE_H
