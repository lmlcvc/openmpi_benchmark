#include "scan_benchmark.h"

ScanBenchmark::ScanBenchmark(std::vector<ArgumentEntry> args, int rank)
{
    m_rank = rank;
    parseArguments(args);

    m_sndBufferBytes = m_sndBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower)); // XXX: check buffer size implementation
    m_rcvBufferBytes = m_rcvBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower));

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
    std::fill(m_bufferSnd, m_bufferSnd + m_sndBufferBytes, 0);

    m_bufferRcv = static_cast<int8_t *>(memRcv);

    if (posix_memalign(&memSnd, pageSize, m_sndBufferBytes) != 0 && posix_memalign(&memRcv, pageSize, m_rcvBufferBytes))
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

void ScanBenchmark::run()
{
}
