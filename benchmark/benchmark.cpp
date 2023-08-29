#include "benchmark.h"

timespec Benchmark::diff(timespec start, timespec end)
{
    timespec time_diff;
    if ((end.tv_nsec - start.tv_nsec) < 0)
    {
        time_diff.tv_sec = end.tv_sec - start.tv_sec - 1;
        time_diff.tv_nsec = 1e9 + end.tv_nsec - start.tv_nsec;
    }
    else
    {
        time_diff.tv_sec = end.tv_sec - start.tv_sec;
        time_diff.tv_nsec = end.tv_nsec - start.tv_nsec;
    }
    return time_diff;
}

std::vector<std::pair<int, int>> Benchmark::findSubarrayIndices(std::size_t messageSize)
{
    int subarraySize = std::ceil(0.32 * messageSize);
    int sharedElements = std::ceil(0.15 * messageSize);

    std::vector<std::pair<int, int>> subarrayIndices;

    for (int i = 0; i < 5; ++i)
    {
        int start = i * (subarraySize - sharedElements);
        int end = start + subarraySize;
        subarrayIndices.push_back(std::make_pair(start, end));
    }

    return subarrayIndices;
}

std::pair<double, double> Benchmark::calculateThroughput(timespec startTime, timespec endTime, std::size_t bytesTransferred, std::size_t iterations)
{
    timespec elapsedTime = diff(startTime, endTime);
    double elapsedSecs = elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9);

    double avgRtt = elapsedSecs / iterations;
    double avgThroughput = (bytesTransferred * 8.0) / (elapsedSecs * 1e6);

    return std::make_pair(avgRtt, avgThroughput);
}

void Benchmark::warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, ReadoutUnit *ru, BuilderUnit *bu,
                                    int ruRank, int buRank, int processRank)
{
    std::size_t subarrayCount = subarrayIndices.size();
    std::size_t subarraySize;

    if (m_rank == ruRank)
    {
        int8_t *bufferSnd = ru->getBuffer();

        for (std::size_t i = 0; i < subarrayCount; i++)
        {
            subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
            MPI_Send(bufferSnd + subarrayIndices[i].first, subarraySize, MPI_BYTE, ruRank, 0, MPI_COMM_WORLD);
        }
    }
    else if (m_rank == buRank)
    {
        int8_t *bufferRcv = bu->getBuffer();

        for (std::size_t i = 0; i < subarrayCount; i++)
        {
            subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
            MPI_Recv(bufferRcv + subarrayIndices[i].first, subarraySize, MPI_BYTE, buRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }
}

void Benchmark::performWarmup()
{
    int buRank;
    timespec startTime, endTime;
    double throughput;
    const std::size_t bufferBytes = m_readoutUnit->getBufferBytes();
    std::size_t messageSize = bufferBytes / 10;
    std::size_t transferredSize = 0;

    std::vector<std::pair<int, int>> subarrayIndices = findSubarrayIndices(bufferBytes);

    clock_gettime(CLOCK_MONOTONIC, &startTime);

    for (int ruRank = 0; ruRank < m_nodesCount; ruRank++)
    {
        buRank = (1 + ruRank) % m_nodesCount;
        if (m_rank == ruRank || m_rank == buRank)
        {
            std::pair<std::size_t, std::size_t> result = CommunicationInterface::unitsBlockingCommunication(m_readoutUnit.get(), m_builderUnit.get(), ruRank, buRank, m_rank,
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
            warmupCommunication(subarrayIndices, m_readoutUnit.get(), m_builderUnit.get(), ruRank, buRank, m_rank);
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
            std::pair<std::size_t, std::size_t> result = CommunicationInterface::unitsBlockingCommunication(m_readoutUnit.get(), m_builderUnit.get(), ruRank, buRank, m_rank,
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
