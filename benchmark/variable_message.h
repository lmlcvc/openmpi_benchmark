#ifndef BENCHMARKVARIABLEMESSAGE_H
#define BENCHMARKVARIABLEMESSAGE_H

#include <random>

#include "continuous_benchmark.h"

class BenchmarkVariableMessage : public ContinuousBenchmark
{
public:
    BenchmarkVariableMessage(std::vector<ArgumentEntry> args, int rank, CommunicationType commType);
    virtual ~BenchmarkVariableMessage() override {}

    void run() override;

private:
    void initMessageSizes();
    void printIterationInfo(timespec startTime, timespec endTime, std::size_t transferredSize) override;
    void parseArguments(std::vector<ArgumentEntry> args) override;

    std::size_t m_messageSizeVariants = 100;
};

#endif // BENCHMARKVARIABLEMESSAGE_H
