#ifndef BENCHMARKFIXEDMESSAGE_H
#define BENCHMARKFIXEDMESSAGE_H

#include "continuous_benchmark.h"

class BenchmarkFixedMessage : public ContinuousBenchmark
{
public:
    BenchmarkFixedMessage(std::vector<ArgumentEntry> args, CommunicationType commType);
    virtual ~BenchmarkFixedMessage() override {}

private:
    void parseArguments(std::vector<ArgumentEntry> args) override;
};

#endif // BENCHMARKFIXEDMESSAGE_H
