#ifndef CONTINUOUSBENCHMARK_H
#define CONTINUOUSBENCHMARK_H

#include "benchmark.h"

class ContinuousBenchmark : public Benchmark
{
public:
    void run() override;

protected:
    virtual void printIterationInfo(timespec startTime, timespec endTime, std::size_t transferredSize, std::size_t errorMessagesCount) = 0;

    CommunicationType m_commType = COMM_UNDEFINED;
    std::size_t m_messageSize = -1;
    std::vector<std::size_t> m_messageSizes;

    const std::size_t minMessageSize = 1e4;
    std::size_t m_syncIterations = 1e4;       // TODO: calculate or parametrise
};

#endif // CONTINUOUSBENCHMARK_H