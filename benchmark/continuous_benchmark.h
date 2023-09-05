#ifndef CONTINUOUSBENCHMARK_H
#define CONTINUOUSBENCHMARK_H

#include "benchmark.h"

struct UnitInfo
{
    int rank;
    std::string id;
};

class ContinuousBenchmark : public Benchmark
{
public:
    void run() override;
    void performWarmup() override;
    void warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int ruRank, int buRank) override;

protected:
    void initUnitLists();
    virtual void printIterationInfo(timespec startTime, timespec endTime, std::string& ruId, std::string& buId,
                                    std::size_t transferredSize, std::size_t errorMessagesCount) = 0;

    CommunicationType m_commType = COMM_UNDEFINED;
    std::size_t m_messageSize = -1;
    std::vector<std::size_t> m_messageSizes;

    const std::size_t minMessageSize = 1e4;
    std::size_t m_syncIterations;

    int m_nodesCount;
    std::size_t m_currentPhase = 0;

    std::unique_ptr<Unit> m_unit;
    std::vector<UnitInfo> m_readoutUnits;
    std::vector<UnitInfo> m_builderUnits;

    std::size_t m_ruBufferBytes = 1e7;
    std::size_t m_buBufferBytes = 1e7;
};

#endif // CONTINUOUSBENCHMARK_H