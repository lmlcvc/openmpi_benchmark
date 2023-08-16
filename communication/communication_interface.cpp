#include "communication_interface.h"

// TODO: handle other types of statuses
std::size_t CommunicationInterface::blockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                          std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                          std::size_t messageSize, int rank, std::size_t iterations, std::size_t *transferredSize)
{
    std::vector<MPI_Status> statuses(iterations);
    std::size_t sendOffset = 0, recvOffset = 0;
    std::size_t remainingSize, wrapSize;
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
            MPI_Status status1, status2;

            if (recvOffset + messageSize > rcvBufferBytes)
            {
                remainingSize = rcvBufferBytes - recvOffset;
                wrapSize = messageSize - remainingSize;

                MPI_Recv(bufferRcv + recvOffset, remainingSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status1);
                recvOffset = 0;
                MPI_Recv(bufferRcv + recvOffset, wrapSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status2);

                if (status1.MPI_ERROR != MPI_SUCCESS)
                    statuses[i].MPI_ERROR = status1.MPI_ERROR;
                else if (status2.MPI_ERROR != MPI_SUCCESS)
                    statuses[i].MPI_ERROR = status2.MPI_ERROR;
            }
            else
                MPI_Recv(bufferRcv + recvOffset, messageSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &statuses[i]);

            recvOffset = (recvOffset + messageSize) % rcvBufferBytes;
        }
    }

    errorMessageCount = std::count_if(statuses.begin(), statuses.end(),
                                      [](const MPI_Status &status)
                                      { return status.MPI_ERROR != MPI_SUCCESS; });

    *transferredSize -= messageSize * errorMessageCount;

    return errorMessageCount;
}

std::size_t CommunicationInterface::variableBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                                  std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                                  std::vector<std::size_t> messageSizes, int rank, std::size_t iterations, std::size_t *transferredSize)
{
    std::vector<MPI_Status> statuses(iterations);
    std::size_t sendOffset = 0, recvOffset = 0;
    std::size_t messageSize, remainingSize, wrapSize;

    std::random_device rd;
    std::mt19937 generator(rd());
    std::uniform_int_distribution<std::size_t> sizeDistribution(0, messageSizes.size() - 1);

    if (rank == 0)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            std::size_t messageSize = messageSizes[sizeDistribution(generator)];

            if (sendOffset + messageSize > sndBufferBytes)
            {
                remainingSize = sndBufferBytes - sendOffset;
                wrapSize = messageSize - remainingSize;

                MPI_Send(bufferSnd + sendOffset, remainingSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
                sendOffset = 0;
                MPI_Send(bufferSnd + sendOffset, wrapSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);
            }
            else
                MPI_Send(bufferSnd + sendOffset, messageSize, MPI_BYTE, 1, 0, MPI_COMM_WORLD);

            sendOffset = (sendOffset + messageSize) % sndBufferBytes;
        }
    }
    else if (rank == 1)
    {
        for (std::size_t i = 0; i < iterations; i++)
        {
            MPI_Status status1, status2;

            if (recvOffset + messageSize > rcvBufferBytes)
            {
                remainingSize = rcvBufferBytes - recvOffset;
                wrapSize = messageSize - remainingSize;

                MPI_Recv(bufferRcv + recvOffset, remainingSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status1);
                recvOffset = 0;
                MPI_Recv(bufferRcv + recvOffset, wrapSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &status2);

                if (status1.MPI_ERROR != MPI_SUCCESS)
                    statuses[i].MPI_ERROR = status1.MPI_ERROR;
                else if (status2.MPI_ERROR != MPI_SUCCESS)
                    statuses[i].MPI_ERROR = status2.MPI_ERROR;
            }
            else
                MPI_Recv(bufferRcv + recvOffset, messageSize, MPI_BYTE, 0, 0, MPI_COMM_WORLD, &statuses[i]);

            recvOffset = (recvOffset + messageSize) % rcvBufferBytes;
            if (statuses[i].MPI_ERROR == MPI_SUCCESS)
                *transferredSize += messageSize;
        }
    }

    return std::count_if(statuses.begin(), statuses.end(),
                         [](const MPI_Status &status)
                         { return status.MPI_ERROR != MPI_SUCCESS; });
}
