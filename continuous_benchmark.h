#ifndef CONTINUOUSBENCHMARK_H
#define CONTINUOUSBENCHMARK_H

#include "benchmark.h"

class ContinuousBenchmark : public Benchmark
{
public:
    //ContinuousBenchmark(std::vector<ArgumentEntry> args, int rank);
    virtual ~ContinuousBenchmark() override {}

    //void run() override;

protected:
    void parseArguments(std::vector<ArgumentEntry> args) override;
    void allocateMemory() override;

    const std::size_t minMessageSize = 1e4;

private:
    // TODO: move these and their usage to fixed message class
    // TODO: adapt allocateMemory to use full sizes instead of products
    std::size_t m_messageSize;   // message size in bytes
    std::size_t m_sndBufferSize; // circular buffer sizes (in messages)
    std::size_t m_rcvBufferSize;
};

#endif // CONTINUOUSBENCHMARK_H