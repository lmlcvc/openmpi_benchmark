#include "continuous_benchmark.h"

void ContinuousBenchmark::run()
{
    // a RU and BU will exist on every rank

    // init a circular vector of indexes for the BUs

    // for every RU (in order) perform communication and measure what is to be measured

    int buRank;
    timespec startTime, endTime;
    std::size_t errorMessageCount = -1;
    std::size_t transferredSize = -1;

    for (int ruRank = 0; ruRank < m_nodesCount; ruRank++)
    {
        buRank = (m_currentPhase + ruRank) % m_nodesCount;

        transferredSize = 0;
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        if (m_rank == ruRank || m_rank == buRank)
        {
            if (m_commType == COMM_FIXED_BLOCKING)
            {
                std::pair<std::size_t, std::size_t> result = CommunicationInterface::unitsBlockingCommunication(m_readoutUnit.get(), m_builderUnit.get(), ruRank, buRank, m_rank,
                                                                                                                m_messageSize, m_iterations);
                errorMessageCount = result.first;
                transferredSize = result.second;
            }
            else if (m_commType == COMM_FIXED_NONBLOCKING)
            {
                std::pair<std::size_t, std::size_t> result = CommunicationInterface::nonBlockingCommunication(m_bufferSnd, m_bufferRcv, m_sndBufferBytes, m_rcvBufferBytes,
                                                                                                              m_messageSize, m_rank, m_iterations, m_syncIterations);
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

            if (m_rank == ruRank)
                printIterationInfo(startTime, endTime, ruRank, buRank, transferredSize, errorMessageCount);
        }
    }
    m_currentPhase = (m_currentPhase + 1) % m_nodesCount;
    // TODO: implement phase syncing
}
