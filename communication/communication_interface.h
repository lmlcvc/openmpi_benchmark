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
    std::pair<std::size_t, std::size_t> twoRankBlockingCommunication(int8_t *bufferSnd, int8_t *bufferRcv,
                                                                     std::size_t sndBufferBytes, std::size_t rcvBufferBytes,
                                                                     std::size_t messageSize, int rank, std::size_t iterations);

    std::pair<std::size_t, std::size_t> blockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                              std::size_t messageSize, std::size_t iterations);

    std::pair<std::size_t, std::size_t> nonBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                 std::size_t messageSize, std::size_t iterations, std::size_t syncIterations);

    std::pair<std::size_t, std::size_t> variableBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                      std::vector<std::size_t> messageSizes, std::size_t iterations);

    std::pair<std::size_t, std::size_t> variableNonBlockingCommunication(Unit *unit, int ruRank, int buRank, int processRank,
                                                                         std::vector<std::size_t> messageSizes, std::size_t iterations, std::size_t syncIterations);
};

#endif // COMMUNICATIONINTERFACE_H
