#include "communication_interface.h"

std::pair<std::size_t, std::size_t> CommunicationInterface::blockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                                                  std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                                                  std::size_t messageSize, int rank, std::size_t iterations)
{
    std::vector<MPI_Status> statuses(iterations);
    std::size_t sendOffset = 0, recvOffset = 0;

    std::size_t errorMessageCount = 0;
    std::size_t transferredSize = messageSize * iterations;

    if (rank == 0)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            if (sendOffset + messageSize > sndBufferBytes)
                sendOffset = 0;

            MPI_Send(bufferSnd + sendOffset, messageSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);

            sendOffset = (sendOffset + messageSize) % sndBufferBytes;
        }
    }
    else if (rank == 1)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            if (recvOffset + messageSize > rcvBufferBytes)
                recvOffset = 0;

            MPI_Recv(bufferRcv + recvOffset, messageSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &statuses[i]);

            recvOffset = (recvOffset + messageSize) % sndBufferBytes;
        }
    }

    errorMessageCount = std::count_if(statuses.begin(), statuses.end(),
                                      [](const MPI_Status &status)
                                      { return status.MPI_ERROR != MPI_SUCCESS; });

    transferredSize -= messageSize * errorMessageCount;

    // Return both error message count and transferred size.
    return std::make_pair(errorMessageCount, transferredSize);
}

// TODO: don't ignore status
std::pair<std::size_t, std::size_t> CommunicationInterface::unitsBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                                       std::size_t messageSize, std::size_t iterations)
{

    std::vector<MPI_Status> statuses(iterations);

    std::size_t errorMessageCount = 0;
    std::size_t transferredSize = messageSize * iterations;

    if (processRank == ruRank)
    {
        int8_t *bufferSnd = unit->getBuffer();
        const std::size_t sndBufferBytes = unit->getBufferBytes();
        std::size_t sendOffset = 0;

        std::cout << "Sending from " << ruRank << " to " << buRank << std::endl;

        for (std::size_t i = 0; i < iterations; i++)
        {
            if (sendOffset + messageSize > sndBufferBytes)
                sendOffset = 0;

            MPI_Send(bufferSnd + sendOffset, messageSize, MPI_BYTE, buRank, 0, MPI_COMM_WORLD);

            sendOffset = (sendOffset + messageSize) % sndBufferBytes;
        }
    }
    else if (processRank == buRank)
    {
        int8_t *bufferRcv = unit->getBuffer();
        const std::size_t rcvBufferBytes = unit->getBufferBytes();
        std::size_t recvOffset = 0;

        for (std::size_t i = 0; i < iterations; i++)
        {
            if (recvOffset + messageSize > rcvBufferBytes)
                recvOffset = 0;

            MPI_Recv(bufferRcv + recvOffset, messageSize, MPI_BYTE, ruRank, 0, MPI_COMM_WORLD, &statuses[i]);

            recvOffset = (recvOffset + messageSize) % rcvBufferBytes;
        }
    }

    errorMessageCount = std::count_if(statuses.begin(), statuses.end(),
                                      [](const MPI_Status &status)
                                      { return status.MPI_ERROR != MPI_SUCCESS; });

    transferredSize -= messageSize * errorMessageCount;

    // Return both error message count and transferred size.
    return std::make_pair(errorMessageCount, transferredSize);
}

std::pair<std::size_t, std::size_t> CommunicationInterface::unitsNonBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                                          std::size_t messageSize, std::size_t iterations, std::size_t syncIterations)
{
    std::vector<MPI_Request> sendRequests(iterations);
    std::vector<MPI_Request> recvRequests(iterations);
    std::vector<MPI_Status> statuses(iterations);

    std::size_t errorMessageCount = 0;
    std::size_t transferredSize = messageSize * iterations;

    if (processRank == ruRank)
    {
        int8_t *bufferSnd = unit->getBuffer();
        std::size_t sndBufferBytes = unit->getBufferBytes();
        std::size_t sendOffset = 0;

        for (std::size_t i = 0; i < iterations; i++)
        {
            if (sendOffset + messageSize > sndBufferBytes)
                sendOffset = 0;

            MPI_Isend(bufferSnd + sendOffset, messageSize, MPI_BYTE, buRank, 0, MPI_COMM_WORLD, &sendRequests[i]);

            sendOffset = (sendOffset + messageSize) % sndBufferBytes;

            if (iterations % syncIterations == 0)
                MPI_Waitall(i + 1, sendRequests.data(), statuses.data());
        }
    }
    else if (processRank == buRank)
    {
        int8_t *bufferRcv = unit->getBuffer();
        std::size_t rcvBufferBytes = unit->getBufferBytes();
        std::size_t recvOffset = 0;

        for (std::size_t i = 0; i < iterations; i++)
        {
            if (recvOffset + messageSize > rcvBufferBytes)
                recvOffset = 0;

            MPI_Irecv(bufferRcv + recvOffset, messageSize, MPI_BYTE, ruRank, 0, MPI_COMM_WORLD, &recvRequests[i]);

            recvOffset = (recvOffset + messageSize) % rcvBufferBytes;
        }
    }

    if (processRank == ruRank)
        MPI_Waitall(iterations, sendRequests.data(), statuses.data());

    errorMessageCount = std::count_if(statuses.begin(), statuses.end(),
                                      [](const MPI_Status &status)
                                      { return status.MPI_ERROR != MPI_SUCCESS; });

    transferredSize -= messageSize * errorMessageCount;

    return std::make_pair(errorMessageCount, transferredSize);
}

// TODO: don't ignore status
std::pair<std::size_t, std::size_t> CommunicationInterface::unitsVariableBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                                               std::vector<std::size_t> messageSizes, std::size_t iterations)
{
    std::vector<MPI_Status> statuses(iterations);

    std::size_t transferredSize = 0;

    if (processRank == ruRank)
    {
        int8_t *bufferSnd = unit->getBuffer();
        const std::size_t sndBufferBytes = unit->getBufferBytes();
        std::size_t sendOffset = 0;

        int sndMessageSize;
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<std::size_t> sizeDistribution(0, messageSizes.size() - 1);

        for (std::size_t i = 0; i < iterations; i++)
        {
            sndMessageSize = static_cast<int>(messageSizes[sizeDistribution(generator)]);

            MPI_Send(&sndMessageSize, 1, MPI_INT, buRank, 0, MPI_COMM_WORLD); // Communicate message size over network

            if (sendOffset + sndMessageSize > sndBufferBytes)
                sendOffset = 0;

            MPI_Send(bufferSnd + sendOffset, sndMessageSize, MPI_BYTE, buRank, 0, MPI_COMM_WORLD);

            sendOffset = (sendOffset + sndMessageSize) % sndBufferBytes;
        }
    }
    else if (processRank == buRank)
    {
        int8_t *bufferRcv = unit->getBuffer();
        const std::size_t rcvBufferBytes = unit->getBufferBytes();
        std::size_t recvOffset = 0;
        std::size_t rcvMessageSize;

        for (std::size_t i = 0; i < iterations; i++)
        {
            MPI_Recv(&rcvMessageSize, 1, MPI_INT, ruRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Receive the messageSize from rank 0

            if (recvOffset + rcvMessageSize > rcvBufferBytes)
                recvOffset = 0;

            MPI_Recv(bufferRcv + recvOffset, rcvMessageSize, MPI_BYTE, ruRank, 0, MPI_COMM_WORLD, &statuses[i]);

            transferredSize += rcvMessageSize;
            recvOffset = (recvOffset + rcvMessageSize) % rcvBufferBytes;
        }
    }

    std::size_t errorMessageCount = std::count_if(statuses.begin(), statuses.end(),
                                                  [](const MPI_Status &status)
                                                  { return status.MPI_ERROR != MPI_SUCCESS; });

    return std::make_pair(errorMessageCount, transferredSize);
}

std::pair<std::size_t, std::size_t> CommunicationInterface::variableNonBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                                             std::vector<std::size_t> messageSizes, std::size_t iterations, std::size_t syncIterations)
{
    std::vector<MPI_Request> sendRequests(iterations);
    std::vector<MPI_Request> recvRequests(iterations);
    std::vector<MPI_Status> statuses(iterations);

    std::size_t errorMessageCount = 0;
    std::size_t transferredSize = 0;

    if (processRank == ruRank)
    {
        int8_t *bufferSnd = unit->getBuffer();
        std::size_t sndBufferBytes = unit->getBufferBytes();
        std::size_t sendOffset = 0;

        int sndMessageSize;
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<std::size_t> sizeDistribution(0, messageSizes.size() - 1);

        for (std::size_t i = 0; i < iterations; i++)
        {
            // Communicate message size over network
            sndMessageSize = static_cast<int>(messageSizes[sizeDistribution(generator)]);

            MPI_Send(&sndMessageSize, 1, MPI_INT, buRank, 0, MPI_COMM_WORLD);

            // Send message from appropriate place in buffer
            if (sendOffset + sndMessageSize > sndBufferBytes)
                sendOffset = 0;

            MPI_Isend(bufferSnd + sendOffset, sndMessageSize, MPI_BYTE, buRank, 0, MPI_COMM_WORLD, &sendRequests[i]);

            sendOffset = (sendOffset + sndMessageSize) % sndBufferBytes;

            // Wait sync after syncIterations
            if (iterations % syncIterations == 0)
                MPI_Waitall(i + 1, sendRequests.data(), statuses.data());
        }
    }
    else if (processRank == buRank)
    {
        int8_t *bufferRcv = unit->getBuffer();
        std::size_t rcvBufferBytes = unit->getBufferBytes();
        std::size_t recvOffset = 0;
        std::size_t rcvMessageSize;

        for (std::size_t i = 0; i < iterations; i++)
        {
            MPI_Recv(&rcvMessageSize, 1, MPI_INT, ruRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Receive the messageSize from rank 0

            if (recvOffset + rcvMessageSize > rcvBufferBytes)
                recvOffset = 0;

            MPI_Irecv(bufferRcv + recvOffset, rcvMessageSize, MPI_BYTE, ruRank, 0, MPI_COMM_WORLD, &recvRequests[i]);

            recvOffset = (recvOffset + rcvMessageSize) % rcvBufferBytes;

            if (statuses.at(i).MPI_ERROR == MPI_SUCCESS)
                transferredSize += rcvMessageSize;
        }
    }

    if (processRank == ruRank)
        MPI_Waitall(iterations, sendRequests.data(), statuses.data());


    errorMessageCount = std::count_if(statuses.begin(), statuses.end(),
                                      [](const MPI_Status &status)
                                      { return status.MPI_ERROR != MPI_SUCCESS; });

    return std::make_pair(errorMessageCount, transferredSize);
}
