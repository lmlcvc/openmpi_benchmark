#include "continuous_benchmark.h"

void ContinuousBenchmark::run()
{
    timespec startTime, endTime;
    std::size_t errorMessageCount = -1;
    std::size_t transferredSize = -1;

    // Reset counters
    transferredSize = 0;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    if (m_commType == COMM_FIXED_BLOCKING)
    {
        std::pair<std::size_t, std::size_t> result = CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                                                   m_messageSize, m_rank, m_iterations);
        errorMessageCount = result.first;
        transferredSize = result.second;
    }
    else if (m_commType == COMM_FIXED_NONBLOCKING)
    {
        // TODO: change to nonblocking when implemented
        std::pair<std::size_t, std::size_t> result = CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                                                   m_messageSize, m_rank, m_iterations);
        errorMessageCount = result.first;
        transferredSize = result.second;
    }
    else if (m_commType == COMM_VARIABLE_BLOCKING)
    {
        std::pair<std::size_t, std::size_t> result = CommunicationInterface::variableBlockingCommunication(m_bufferSnd, m_bufferRcv,
                                                                                                           m_sndBufferBytes, m_rcvBufferBytes,
                                                                                                           m_messageSizes, m_rank, m_iterations);
        errorMessageCount = result.first;
        transferredSize = result.second;
    }

    else if (m_commType == COMM_VARIABLE_NONBLOCKING)
    {
        std::pair<std::size_t, std::size_t> result = CommunicationInterface::blockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                                                   m_messageSize, m_rank, m_iterations);
        errorMessageCount = result.first;
        transferredSize = result.second;
    }

    clock_gettime(CLOCK_MONOTONIC, &endTime);

    printIterationInfo(startTime, endTime, transferredSize, errorMessageCount);
}
