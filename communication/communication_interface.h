#ifndef COMMUNICATIONINTERFACE_H
#define COMMUNICATIONINTERFACE_H

#include <mpi.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <random>

class CommunicationInterface
{
public:
    std::size_t blockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                      std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                      std::size_t messageSize, int rank, std::size_t iterations, std::size_t *transferredSize);
    // std::size_t nonBlockingCommunication(std::size_t messageSize, std::size_t iterations = -1);

    std::size_t variableBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                              std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                              std::vector<std::size_t> messageSizes, int rank, std::size_t iterations, std::size_t *transferredSize);
    // std::size_t variableNonBlockingCommunication(std::vector<std::size_t> messageSizes, std::size_t iterations = -1);
};

#endif // COMMUNICATIONINTERFACE_H
