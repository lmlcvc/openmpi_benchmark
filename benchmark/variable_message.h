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
    void printIterationInfo(timespec startTime, timespec endTime, std::string& ruId, std::string& buId,
                            std::size_t transferredSize, std::size_t errorMessagesCount) override;
    void parseArguments(std::vector<ArgumentEntry> args) override;

    std::size_t m_messageSizeVariants = 100;
};

#endif // BENCHMARKVARIABLEMESSAGE_H
