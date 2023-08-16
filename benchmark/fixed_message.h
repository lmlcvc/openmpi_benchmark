#ifndef BENCHMARKFIXEDMESSAGE_H
#define BENCHMARKFIXEDMESSAGE_H

#include "continuous_benchmark.h"

class BenchmarkFixedMessage : public ContinuousBenchmark
{
public:
    BenchmarkFixedMessage(std::vector<ArgumentEntry> args, int rank);
    virtual ~BenchmarkFixedMessage() override {}

    void run() override;

private:
    void parseArguments(std::vector<ArgumentEntry> args) override;

    std::size_t m_messageSize = 1e4; // message size in bytes
};

#endif // BENCHMARKFIXEDMESSAGE_H
