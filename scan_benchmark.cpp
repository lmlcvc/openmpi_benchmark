#include "scan_benchmark.h"

ScanBenchmark::ScanBenchmark(std::vector<ArgumentEntry> args, int rank)
{
    m_rank = rank;
    parseArguments(args);

    allocateMemory();
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

void ScanBenchmark::allocateMemory()
{
    const long pageSize = sysconf(_SC_PAGESIZE);

    m_bufferSnd = static_cast<int8_t *>(memSnd);
    std::size_t sndAlignSize = m_sndBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower));
    std::fill(m_bufferSnd, m_bufferSnd + sndAlignSize, 0);

    m_bufferRcv = static_cast<int8_t *>(memRcv);
    std::size_t rcvAlignSize = m_rcvBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower));

    if (posix_memalign(&memSnd, pageSize, sndAlignSize) != 0 && posix_memalign(&memRcv, pageSize, m_rcvBufferSize))
    {
        std::cerr << "Memory allocation failed" << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    MPI_Bcast(&memSnd, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memSndPtr(m_rank == 0 ? memSnd : nullptr, &free);

    MPI_Bcast(&memRcv, sizeof(void *), MPI_BYTE, 0, MPI_COMM_WORLD);
    std::unique_ptr<void, decltype(&free)> memRcvPtr(m_rank == 0 ? memRcv : nullptr, &free);
}

void ScanBenchmark::performWarmup()
{
    // TODO: move function to benchmark.cpp

    // TODO: handle case when send buffer is smaller than whatever is used here - will be done by passing buffer size in bytes

    timespec startTime, endTime;
    double throughput;
    std::size_t messageSize = 1e6;

    std::size_t sndAlignSize = m_sndBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower));
    std::vector<std::pair<int, int>> subarrayIndices = findSubarrayIndices(m_sndBufferSize * sndAlignSize); // FIXME: should be whatever the buffer size is
    // TODO: store buffer size in bytes

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    rtCommunication(m_sndBufferSize, m_rcvBufferSize, messageSize, m_warmupIterations);
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, messageSize, m_warmupIterations);

    std::cout << "\nPre-warmup throughput: " << throughput << " Mbit/s" << std::endl;

    warmupCommunication(m_bufferSnd, subarrayIndices, m_rank);

    clock_gettime(CLOCK_MONOTONIC, &startTime);
    rtCommunication(m_sndBufferSize, m_rcvBufferSize, messageSize, m_warmupIterations);
    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, messageSize, m_warmupIterations);

    std::cout << "Post-warmup throughput: " << throughput << " Mbit/s\n"
              << std::endl;
}

void ScanBenchmark::run()
{
}
