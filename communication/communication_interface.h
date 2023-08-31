#ifndef COMMUNICATIONINTERFACE_H
#define COMMUNICATIONINTERFACE_H

#include <mpi.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>
#include <random>

#include "../unit/unit.h"

class CommunicationInterface
{
public:
    // TODO: refactor function names

    // TODO: get buffers info from benchmark object in blocking comm
    std::pair<std::size_t, std::size_t> blockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                              std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                              std::size_t messageSize, int rank, std::size_t iterations);

    // specified ranks
    std::pair<std::size_t, std::size_t> unitsBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                   std::size_t messageSize, std::size_t iterations);

    std::pair<std::size_t, std::size_t> unitsNonBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                      std::size_t messageSize, std::size_t iterations, std::size_t syncIterations);

    std::pair<std::size_t, std::size_t> unitsVariableBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                           std::vector<std::size_t> messageSizes, std::size_t iterations);

    std::pair<std::size_t, std::size_t> variableNonBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                         std::vector<std::size_t> messageSizes, std::size_t iterations, std::size_t syncIterations);
};

#endif // COMMUNICATIONINTERFACE_H
