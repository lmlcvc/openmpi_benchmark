#include "benchmark.h"

timespec diff(timespec start, timespec end)
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
    // TODO: override for ContinuousVariableMessage
    // XXX: don't need iterations for scan run
    timespec elapsedTime = diff(startTime, endTime);
    double elapsedSecs = elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9);

    double avgRtt = elapsedSecs / iterations;
    double avgThroughput = (bytesTransferred * 8.0) / (elapsedSecs * 1e6);

    return std::make_pair(avgRtt, avgThroughput);
}

void Benchmark::allocateMemory()
{
    const long pageSize = sysconf(_SC_PAGESIZE);

    if (posix_memalign(&memSnd, pageSize, m_sndBufferBytes) != 0 || posix_memalign(&memRcv, pageSize, m_rcvBufferBytes) != 0)
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    m_bufferSnd = static_cast<int8_t *>(memSnd);
    std::fill(m_bufferSnd, m_bufferSnd + m_sndBufferBytes, 0);

    m_bufferRcv = static_cast<int8_t *>(memRcv);

    MPI_Bcast(&memSnd, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memSndPtr(m_rank == 0 ? memSnd : nullptr, &free);

    MPI_Bcast(&memRcv, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memRcvPtr(m_rank == 0 ? memRcv : nullptr, &free);
}

void Benchmark::printRunInfo(double rtt, double throughput, int errorMessagesCount)
{
    if (m_rank)
        return;

    std::cout << std::fixed << std::setprecision(8);

    std::cout << std::right << std::setw(14) << " Avg. RTT"
              << " | " << std::setw(25) << "Throughput"
              << " | " << std::setw(10) << " Errors"
              << std::endl;

    std::cout << std::right << std::setw(12) << rtt << " s"
              << " | " << std::setw(18) << std::fixed << std::setprecision(2) << throughput << " Mbit/s"
              << " | " << std::setw(10) << errorMessagesCount << std::endl
              << std::endl;
}

void Benchmark::warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int8_t rank)
{
    std::size_t subarrayCount = subarrayIndices.size();
    std::size_t subarraySize;

    if (rank == 0)
        std::cout << "Performing warmup...";

    if (rank == 0)
    {
        for (int i = 0; i < subarrayCount; i++)
        {
            subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
            MPI_Send(m_bufferSnd + subarrayIndices[i].first, subarraySize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
        }
    }
    else if (rank == 1)
    {
        for (int i = 0; i < subarrayCount; i++)
        {
            subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
            MPI_Recv(m_bufferRcv + subarrayIndices[i].first, subarraySize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
        }
    }

    if (rank == 0)
        std::cout << "Done." << std::endl;
}

void Benchmark::performWarmup()
{
    timespec startTime, endTime;
    double throughput;
    std::size_t messageSize = m_sndBufferBytes / 10;
    std::size_t transferredSize = 0;

    std::vector<std::pair<int, int>> subarrayIndices = findSubarrayIndices(m_sndBufferBytes);

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    // TODO: monitor errors in warmup
    CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                  messageSize, m_rank, m_warmupIterations, &transferredSize);
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, transferredSize, m_warmupIterations);

    std::cout << "\nPre-warmup throughput: " << throughput << " Mbit/s" << std::endl;

    warmupCommunication(subarrayIndices, m_rank);

    transferredSize = 0;
    clock_gettime(CLOCK_MONOTONIC, &startTime);
    CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                  messageSize, m_rank, m_warmupIterations, &transferredSize);
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, transferredSize, m_warmupIterations);

    std::cout << "Post-warmup throughput: " << throughput << " Mbit/s\n"
              << std::endl;
}
