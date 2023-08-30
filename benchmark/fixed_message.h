#ifndef BENCHMARKFIXEDMESSAGE_H
#define BENCHMARKFIXEDMESSAGE_H

#include "continuous_benchmark.h"

class BenchmarkFixedMessage : public ContinuousBenchmark
{
public:
    BenchmarkFixedMessage(std::vector<ArgumentEntry> args, int rank, int size, CommunicationType commType);
    virtual ~BenchmarkFixedMessage() override {}

private:
    void parseArguments(std::vector<ArgumentEntry> args) override;
    void printIterationInfo(timespec startTime, timespec endTime, std::string ruId, std::string buId,
                            std::size_t transferredSize, std::size_t errorMessagesCount) override;
};

#endif // BENCHMARKFIXEDMESSAGE_H
