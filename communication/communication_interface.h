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
    std::pair<std::size_t, std::size_t> blockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                              std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                              std::size_t messageSize, int rank, std::size_t iterations);

    std::pair<std::size_t, std::size_t> nonBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                                 std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                                 std::size_t messageSize, int rank, std::size_t iterations, std::size_t syncIterations);

    std::pair<std::size_t, std::size_t> variableBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                                      std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                                      std::vector<std::size_t> messageSizes, int rank, std::size_t iterations);

    std::size_t variableNonBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                 std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                 std::vector<std::size_t> messageSizes, int rank, std::size_t iterations, std::size_t syncIterations, std::size_t *transferredSize);
};

#endif // COMMUNICATIONINTERFACE_H
