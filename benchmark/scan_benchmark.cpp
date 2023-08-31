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
