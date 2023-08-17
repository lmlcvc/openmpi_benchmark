#include "communication_interface.h"

// TODO: handle other types of statuses
std::size_t CommunicationInterface::blockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                          std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                          std::size_t messageSize, int rank, std::size_t iterations, std::size_t *transferredSize)
{
    std::vector<MPI_Status> statuses(iterations);
    std::size_t sendOffset = 0, recvOffset = 0;
    std::size_t remainingSize, wrapSize, remainingToReceive, chunkSize;
    std::size_t errorMessageCount = 0;

    *transferredSize = messageSize * iterations;

    if (rank == 0)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            if (sendOffset + messageSize > sndBufferBytes)
            {
                remainingSize = sndBufferBytes - sendOffset;
                wrapSize = messageSize - remainingSize;

                MPI_Send(bufferSnd + sendOffset, remainingSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
                sendOffset = 0;
                MPI_Send(bufferSnd + sendOffset, wrapSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            }
            else
                MPI_Send(bufferSnd + sendOffset, messageSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD); // FIXME: failed to register user buffer

            sendOffset = (sendOffset + messageSize) % sndBufferBytes;
        }
    }
    else if (rank == 1)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            MPI_Status status;
            remainingToReceive = messageSize;

            while (remainingToReceive > 0)
            {
                chunkSize = std::min(remainingToReceive, rcvBufferBytes - recvOffset);

                MPI_Recv(bufferRcv + recvOffset, chunkSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);

                if (status.MPI_ERROR != MPI_SUCCESS)
                    statuses[i].MPI_ERROR = status.MPI_ERROR;

                recvOffset = (recvOffset + chunkSize) % rcvBufferBytes;
                remainingToReceive -= chunkSize;
            }
        }

        errorMessageCount = std::count_if(statuses.begin(), statuses.end(),
                                          [](const MPI_Status &status)
                                          { return status.MPI_ERROR != MPI_SUCCESS; });

        *transferredSize -= messageSize * errorMessageCount;

        return errorMessageCount;
    }

    errorMessageCount = std::count_if(statuses.begin(), statuses.end(),
                                      [](const MPI_Status &status)
                                      { return status.MPI_ERROR != MPI_SUCCESS; });

    *transferredSize -= messageSize * errorMessageCount;

    return errorMessageCount;
}

std::size_t CommunicationInterface::nonBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                             std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                             std::size_t messageSize, int rank, std::size_t iterations, std::size_t *transferredSize)
{
    return 0;
}

std::size_t CommunicationInterface::variableBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                                  std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                                  std::vector<std::size_t> messageSizes, int rank, std::size_t iterations, std::size_t *transferredSize)
{
    std::vector<MPI_Status> statuses(iterations);
    std::size_t sendOffset = 0, recvOffset = 0;
    std::size_t remainingSize, wrapSize, remainingToReceive, chunkSize;

    if (rank == 0)
    {
        int sndMessageSize;
        std::random_device rd;
        std::mt19937 generator(rd());
        std::uniform_int_distribution<std::size_t> sizeDistribution(0, messageSizes.size() - 1);

        for (std::size_t i = 0; i < iterations; i++)
        {
            sndMessageSize = static_cast<int>(messageSizes[sizeDistribution(generator)]);

            MPI_Send(&sndMessageSize, 1, MPI_INT, 1, 0, MPI_COMM_WORLD); // Communicate the messageSize to rank 1

            if (sendOffset + sndMessageSize > sndBufferBytes)
            {
                remainingSize = sndBufferBytes - sendOffset;
                wrapSize = sndMessageSize - remainingSize;

                MPI_Send(bufferSnd + sendOffset, remainingSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
                sendOffset = 0;
                MPI_Send(bufferSnd + sendOffset, wrapSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            }
            else
                MPI_Send(bufferSnd + sendOffset, sndMessageSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);

            sendOffset = (sendOffset + sndMessageSize) % sndBufferBytes;
        }
    }
    else if (rank == 1)
    {
        std::size_t rcvMessageSize;

        for (std::size_t i = 0; i < iterations; i++)
        {
            MPI_Recv(&rcvMessageSize, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE); // Receive the messageSize from rank 0

            MPI_Status status;
            remainingToReceive = rcvMessageSize;

            while (remainingToReceive > 0)
            {
                chunkSize = std::min(remainingToReceive, rcvBufferBytes - recvOffset);

                MPI_Recv(bufferRcv + recvOffset, chunkSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status);

                if (status.MPI_ERROR != MPI_SUCCESS)
                    statuses[i].MPI_ERROR = status.MPI_ERROR;

                recvOffset = (recvOffset + chunkSize) % rcvBufferBytes;
                remainingToReceive -= chunkSize;
            }

            if (statuses[i].MPI_ERROR == MPI_SUCCESS)
                *transferredSize += rcvMessageSize;
        }
    }

    return std::count_if(statuses.begin(), statuses.end(),
                         [](const MPI_Status &status)
                         { return status.MPI_ERROR != MPI_SUCCESS; });
}

std::size_t variableNonBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                             std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                             std::vector<std::size_t> messageSizes, int rank, std::size_t iterations, std::size_t *transferredSize)
{
    return 0;
}
