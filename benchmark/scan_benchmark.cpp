#include "scan_benchmark.h"

ScanBenchmark::ScanBenchmark(std::vector<ArgumentEntry> args, int rank)
{
    int size;
    MPI_Comm_size(MPI_COMM_WORLD, &size);

    if (size != 2)
    {
        if (rank == 0)
            std::cerr << "Scan benchmark requires exactly 2 processes. Exiting." << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    m_rank = rank;
    parseArguments(args);

    m_sndBufferBytes = m_sndBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower));
    m_rcvBufferBytes = m_rcvBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower));

    allocateMemory();
}

void ScanBenchmark::allocateMemory()
{
    const long pageSize = sysconf(_SC_PAGESIZE);

    void *memSnd = nullptr;
    void *memRcv = nullptr;

    if (posix_memalign(&memSnd, pageSize, m_sndBufferBytes) != 0 || posix_memalign(&memRcv, pageSize, m_rcvBufferBytes) != 0)
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    m_bufferSnd = static_cast<int8_t *>(memSnd);
    std::fill(m_bufferSnd, m_bufferSnd + m_sndBufferBytes, 0);

    m_bufferRcv = static_cast<int8_t *>(memRcv);

    m_memSndPtr = buffer_t(memSnd, free);
    m_memRcvPtr = buffer_t(memRcv, free);
}

void ScanBenchmark::parseArguments(std::vector<ArgumentEntry> args)
{
    std::size_t tmp;
    for (const auto &entry : args)
    {
        switch (entry.option)
        {
        case 'b':
            tmp = std::stoul(entry.value);
            m_sndBufferSize = (tmp > 0) ? tmp : m_sndBufferSize;
            break;
        case 'r':
            tmp = std::stoul(entry.value);
            m_rcvBufferSize = (tmp > 0) ? tmp : m_rcvBufferSize;
            break;
        case 'p':
            tmp = std::stoul(entry.value);
            m_maxPower = (tmp > 0) ? tmp : m_maxPower;
            break;
        case 'i':
            tmp = std::stoul(entry.value);
            m_iterations = (tmp > m_minIterations) ? tmp : m_iterations;
            break;
        case 'w':
            tmp = std::stoul(entry.value);
            m_warmupIterations = (tmp > 0) ? tmp : m_warmupIterations;
            break;
        default:
            if (m_rank == 0)
            {
                std::cerr << "Invalid argument: " << entry.option << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
        }
    }
}

void ScanBenchmark::printRunInfo(std::size_t messageSize, double throughput)
{
    if (m_rank)
        return;

    std::cout << std::fixed << std::setprecision(2);
    std::cout << "| " << std::left << std::setw(12) << messageSize
              << " | " << std::setw(19) << throughput
              << " |\n";
}

void ScanBenchmark::warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int ruRank, int buRank)
{
    std::size_t subarrayCount = subarrayIndices.size();
    std::size_t subarraySize;

    if (m_rank == ruRank)
        std::cout << "Performing warmup...";

    if (m_rank == ruRank)
    {
        for (std::size_t i = 0; i < m_warmupIterations; i++)
        {
            for (int i = 0; i < subarrayCount; i++)
            {
                subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
                MPI_Send(m_bufferSnd + subarrayIndices[i].first, subarraySize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            }
        }
    }
    else if (m_rank == buRank)
    {
        for (std::size_t i = 0; i < m_warmupIterations; i++)
        {
            for (int i = 0; i < subarrayCount; i++)
            {
                subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
                MPI_Recv(m_bufferRcv + subarrayIndices[i].first, subarraySize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }

    if (m_rank == ruRank)
        std::cout << "Done." << std::endl;
}

void ScanBenchmark::performWarmup()
{
    timespec startTime, endTime;
    double throughput;
    std::size_t messageSize = m_sndBufferBytes / 10;
    std::size_t transferredSize = 0;

    std::vector<std::pair<int, int>> subarrayIndices = findSubarrayIndices(m_sndBufferBytes);

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    std::pair<std::size_t, std::size_t> result = CommunicationInterface::twoRankBlockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                                                      messageSize, m_rank, m_warmupIterations);
    transferredSize = result.second;

    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, transferredSize, m_warmupIterations);

    if (m_rank == 0)
        std::cout << "\nPre-warmup throughput: " << throughput << " Mbit/s" << std::endl;

    warmupCommunication(subarrayIndices, 0, 1);

    transferredSize = 0;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    result = CommunicationInterface::twoRankBlockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                  messageSize, m_rank, m_warmupIterations);
    transferredSize = result.second;
    clock_gettime(CLOCK_MONOTONIC, &endTime);

    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, transferredSize, m_warmupIterations);

    if (m_rank == 0)
        std::cout << "Post-warmup throughput: " << throughput << " Mbit/s\n"
                  << std::endl;
}

void ScanBenchmark::run()
{
    if (m_rank == 0)
    {
        std::cout << std::fixed << std::setprecision(2);
        std::cout << "| " << std::left << std::setw(12) << "Bytes"
                  << " | " << std::setw(18) << "Throughput [Mbit/s]"
                  << " |\n";
        std::cout << "--------------------------------------\n";
    }

    double avgThroughput;
    std::size_t errorMessageCount = 0;
    timespec startTime, endTime;

    std::size_t currentMessageSize;
    std::size_t transferredSize;

    for (std::size_t power = 0; power <= m_maxPower; power++)
    {
        currentMessageSize = static_cast<std::size_t>(std::pow(2, power));
        transferredSize = 0;
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        std::pair<std::size_t, std::size_t> result = CommunicationInterface::twoRankBlockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                                                          currentMessageSize, m_rank, m_iterations);
        errorMessageCount += result.first;
        transferredSize = result.second;

        clock_gettime(CLOCK_MONOTONIC, &endTime);
        std::tie(std::ignore, avgThroughput) = calculateThroughput(startTime, endTime, transferredSize, m_iterations);

        printRunInfo(currentMessageSize, avgThroughput);
    }

    if (m_rank == 0)
        std::cout << "\nNumber of non-MPI_SUCCESS statuses: " << errorMessageCount << "\n"
                  << std::endl;
}
