#include "continuous_benchmark.h"

void ContinuousBenchmark::run()
{
    performWarmup();

    timespec startTime, endTime;
    std::size_t errorMessageCount = 0;
    std::size_t transferredSize;

    // Reset counters
    transferredSize = 0;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    if (m_commType == COMM_FIXED_BLOCKING)
    {
        errorMessageCount = CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                          m_messageSize, m_rank, m_iterations, &transferredSize);
    }
    else if (m_commType == COMM_FIXED_NONBLOCKING)
    {
        errorMessageCount = CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                          m_messageSize, m_rank, m_iterations, &transferredSize);
    }
    else if (m_commType == COMM_VARIABLE_BLOCKING)
    {
        errorMessageCount = CommunicationInterface::variableBlockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                                  m_messageSizes, m_rank, m_iterations, &transferredSize);
    }
    else if (m_commType == COMM_VARIABLE_NONBLOCKING)
    {
        errorMessageCount = CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                          m_messageSize, m_rank, m_iterations, &transferredSize);
    }

    clock_gettime(CLOCK_MONOTONIC, &endTime);

    if (m_rank == 0)
        printIterationInfo(startTime, endTime, transferredSize, errorMessageCount);
}
