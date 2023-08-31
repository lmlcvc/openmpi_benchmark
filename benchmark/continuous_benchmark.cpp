#include "continuous_benchmark.h"

std::string getNextID(const std::string &currentID)
{
    std::string nextID = currentID;

    if (currentID.empty() || currentID.back() == 'Z')
    {
        nextID += 'A';
    }
    else
    {
        nextID.back() += 1;
    }

    return nextID;
}

void ContinuousBenchmark::initUnitLists()
{
    UnitInfo tmpInfo;
    std::string alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
    std::string currentID = "A";

    for (std::size_t i = 0; i < m_nodesCount; i++)
    {
        tmpInfo.rank = i;
        if (i < m_nodesCount / 2)
        {
            tmpInfo.id = std::to_string(i + 1);
            m_readoutUnits.push_back(tmpInfo);

            if (m_rank == i)
                m_unit->setBufferBytes(m_ruBufferBytes);
        }
        else
        {
            tmpInfo.id = currentID;
            m_builderUnits.push_back(tmpInfo);

            currentID = getNextID(currentID);

            if (m_rank == i)
                m_unit->setBufferBytes(m_buBufferBytes);
        }
    }

    if (m_rank == 0)
    {
        std::cout << "m_readoutUnits:" << std::endl;
        for (const auto &unit : m_readoutUnits)
        {
            std::cout << "Rank: " << unit.rank << ", ID: " << unit.id << std::endl;
        }

        std::cout << "m_builderUnits:" << std::endl;
        for (const auto &unit : m_builderUnits)
        {
            std::cout << "Rank: " << unit.rank << ", ID: " << unit.id << std::endl;
        }
    }
}

void ContinuousBenchmark::warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int ruRank, int buRank)
{
    std::size_t subarrayCount = subarrayIndices.size();
    std::size_t subarraySize;

    if (m_rank == ruRank)
    {
        int8_t *bufferSnd = m_unit->getBuffer();

        for (std::size_t i = 0; i < subarrayCount; i++)
        {
            subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
            MPI_Send(bufferSnd + subarrayIndices[i].first, subarraySize, MPI_BYTE, buRank, 0, MPI_COMM_WORLD);
        }
    }
    else if (m_rank == buRank)
    {
        int8_t *bufferRcv = m_unit->getBuffer();

        for (std::size_t i = 0; i < subarrayCount; i++)
        {
            subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
            MPI_Recv(bufferRcv + subarrayIndices[i].first, subarraySize, MPI_BYTE, ruRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}

void ContinuousBenchmark::performWarmup()
{
    int buRank;
    timespec startTime, endTime;
    double throughput;
    const std::size_t bufferBytes = m_unit->getBufferBytes();
    std::size_t messageSize = bufferBytes / 10;
    std::size_t transferredSize = 0;

    std::vector<std::pair<int, int>> subarrayIndices = findSubarrayIndices(bufferBytes);

    clock_gettime(CLOCK_MONOTONIC, &startTime);

    for (int ruRank = 0; ruRank < m_nodesCount; ruRank++)
    {
        buRank = (1 + ruRank) % m_nodesCount;
        if (m_rank == ruRank || m_rank == buRank)
        {
            std::pair<std::size_t, std::size_t> result = CommunicationInterface::unitsBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                                            messageSize, m_warmupIterations);
            transferredSize += result.second;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, transferredSize, m_warmupIterations);

    if (m_rank == 0)
        std::cout << "\nAvg. pre-warmup throughput: " << throughput << " Mbit/s" << std::endl
                  << "Performing warmup...";

    for (int ruRank = 0; ruRank < m_nodesCount; ruRank++)
    {
        buRank = (1 + ruRank) % m_nodesCount;
        if (m_rank == ruRank || m_rank == buRank)
            warmupCommunication(subarrayIndices, ruRank, buRank);
    }

    if (m_rank == 0)
        std::cout << "Done." << std::endl;

    transferredSize = 0;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    for (int ruRank = 0; ruRank < m_nodesCount; ruRank++)
    {
        buRank = (1 + ruRank) % m_nodesCount;
        if (m_rank == ruRank || m_rank == buRank)
        {
            std::cout << m_rank << std::endl;
            std::pair<std::size_t, std::size_t> result = CommunicationInterface::unitsBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                                            messageSize, m_warmupIterations);
            transferredSize += result.second;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &endTime);

    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, transferredSize, m_warmupIterations);

    if (m_rank == 0)
        std::cout << "Avg. post-warmup throughput: " << throughput << " Mbit/s\n"
                  << std::endl;
}

void ContinuousBenchmark::run()
{
    int ruRank, buRank;
    int ruRankIndex, buRankIndex;
    std::string ruId, buId;
    timespec startTime, endTime;
    std::size_t errorMessageCount = -1;
    std::size_t transferredSize = -1;

    for (int i = 0; i < m_readoutUnits.size(); i++)
    {
        ruRankIndex = i;
        ruRank = m_readoutUnits.at(ruRankIndex).rank;
        ruId = m_readoutUnits.at(ruRankIndex).id;

        buRankIndex = (m_currentPhase + i) % m_builderUnits.size();
        buRank = m_builderUnits.at(buRankIndex).rank;
        buId = m_builderUnits.at(ruRankIndex).rank;

        transferredSize = 0;
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        if (m_rank == ruRank || m_rank == buRank)
        {
            if (m_commType == COMM_FIXED_BLOCKING)
            {
                std::pair<std::size_t, std::size_t> result = CommunicationInterface::unitsBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                                                m_messageSize, m_iterations);
                errorMessageCount = result.first;
                transferredSize = result.second;
            }
            else if (m_commType == COMM_FIXED_NONBLOCKING)
            {
                std::pair<std::size_t, std::size_t> result = CommunicationInterface::unitsNonBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                                                   m_messageSize, m_iterations, m_syncIterations);

                errorMessageCount = result.first;
                transferredSize = result.second;
            }
            else if (m_commType == COMM_VARIABLE_BLOCKING)
            {
                std::pair<std::size_t, std::size_t> result = CommunicationInterface::unitsVariableBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                                                        m_messageSizes, m_iterations);

                errorMessageCount = result.first;
                transferredSize = result.second;
            }
            else if (m_commType == COMM_VARIABLE_NONBLOCKING)
            {
                std::pair<std::size_t, std::size_t> result = CommunicationInterface::variableNonBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                                                      m_messageSizes, m_iterations, m_syncIterations);
                errorMessageCount = result.first;
                transferredSize = result.second;
            }

            clock_gettime(CLOCK_MONOTONIC, &endTime);

            if (m_rank == buRank)
                printIterationInfo(startTime, endTime, ruId, buId, transferredSize, errorMessageCount);
        }
    }
    m_currentPhase++;
    MPI_Barrier(MPI_COMM_WORLD);
}
