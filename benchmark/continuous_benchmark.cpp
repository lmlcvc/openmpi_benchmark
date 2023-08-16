#include "continuous_benchmark.h"

void ContinuousBenchmark::allocateMemory()
{
    // TODO: move to benchmark
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

void ContinuousBenchmark::run()
{
    performWarmup();

    timespec startTime, endTime;
    std::size_t errorMessageCount = 0;
    std::size_t transferredSize;
    bool running = true;

    while (running)
    {
        // Reset counters
        transferredSize = 0;
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        if (m_commType == COMM_FIXED_BLOCKING)
        {
            errorMessageCount = CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                              m_messageSize, m_rank, m_iterations, &transferredSize);
        }
        else if (m_commType == COMM_VARIABLE_BLOCKING)
        {
            errorMessageCount = CommunicationInterface::variableBlockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                                      m_messageSizes, m_rank, m_iterations, &transferredSize);
        }

        clock_gettime(CLOCK_MONOTONIC, &endTime);

        if (m_rank == 0)
            printIterationInfo(startTime, endTime, transferredSize);

        /*if (sigintReceived)
        {
            // TODO: handle sigint
            // TODO: print elapsed time (implement in ContinuousBenchmark)
            std::exit(EXIT_SUCCESS);
        }*/
    }
}
