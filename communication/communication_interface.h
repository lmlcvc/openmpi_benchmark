#ifndef COMMUNICATIONINTERFACE_H
#define COMMUNICATIONINTERFACE_H

#include <mpi.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <random>

#include "../unit/readout_unit.h"
#include "../unit/builder_unit.h"

class CommunicationInterface
{
public:
    // ranks 0 and 1
    std::pair<std::size_t, std::size_t> blockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                              std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                              std::size_t messageSize, int rank, std::size_t iterations);

    // specified ranks
    std::pair<std::size_t, std::size_t> unitsBlockingCommunication(ReadoutUnit *ru, BuilderUnit *bu, int ruRank, int buRank, int processRank,
                                                                   std::size_t messageSize, std::size_t iterations);

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
