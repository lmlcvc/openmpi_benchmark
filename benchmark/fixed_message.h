#ifndef BENCHMARKFIXEDMESSAGE_H
#define BENCHMARKFIXEDMESSAGE_H

#include "continuous_benchmark.h"

class BenchmarkFixedMessage : public ContinuousBenchmark
{
public:
    BenchmarkFixedMessage(std::vector<ArgumentEntry> args, int rank, CommunicationType commType);
    virtual ~BenchmarkFixedMessage() override {}

    void run() override;

private:
    void parseArguments(std::vector<ArgumentEntry> args) override;
    void printIterationInfo(timespec startTime, timespec endTime, std::size_t transferredSize, std::size_t errorMessagesCount) override;
};

#endif // BENCHMARKFIXEDMESSAGE_H
