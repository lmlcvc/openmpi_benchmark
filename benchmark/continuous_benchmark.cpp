#include "continuous_benchmark.h"

std::string getNextID(const std::string &currentID)
{
    std::string nextID = currentID;

    if (currentID.empty() || currentID.back() == 'Z')
    {
        nextID += 'A';
    }
    else
    {
        nextID.back() += 1;
    }

    return nextID;
}

std::string communicationTypeToString(CommunicationType commType)
{
    switch (commType)
    {
    case COMM_SCAN:
        return "SCAN";
    case COMM_FIXED_BLOCKING:
        return "FIXED_BLOCKING";
    case COMM_FIXED_NONBLOCKING:
        return "FIXED_NONBLOCKING";
    case COMM_VARIABLE_BLOCKING:
        return "VARIABLE_BLOCKING";
    case COMM_VARIABLE_NONBLOCKING:
        return "VARIABLE_NONBLOCKING";
    default:
        return "UNKNOWN";
    }
}

std::string messageSizeToString(CommunicationType commType, std::size_t messageSize)
{
    if (commType == COMM_VARIABLE_BLOCKING || commType == COMM_VARIABLE_NONBLOCKING)
    {
        return "VARIABLE";
    }
    return std::to_string(messageSize);
}

void ContinuousBenchmark::initUnitLists()
{
    UnitInfo tmpInfo;
    std::string currentID = "A";

    for (std::size_t i = 0; i < m_nodesCount; i++)
    {
        tmpInfo.rank = i;

        // RUs
        if (i % 2 == 0)
        {
            tmpInfo.id = std::to_string(m_readoutUnits.size());
            m_readoutUnits.push_back(tmpInfo);

            if (m_rank == i)
            {
                m_unit->setId(tmpInfo.id);
                m_unit->setBufferBytes(m_ruBufferBytes);
                m_unit->setUnitType(UnitType::RU);
                m_unit->ruShift(m_readoutUnits.size() - 1);
            }
        }

        // BUs
        else
        {
            tmpInfo.id = currentID;
            m_builderUnits.push_back(tmpInfo);

            currentID = getNextID(currentID);

            if (m_rank == i)
            {
                m_unit->setId(tmpInfo.id);
                m_unit->setBufferBytes(m_buBufferBytes);
                m_unit->setUnitType(UnitType::BU);
                m_unit->buShift(m_builderUnits.size() - 1);
            }
        }
    }

    if (m_rank == 0)
    {
        std::cout << "\nRUs:" << std::endl;
        for (const auto &unit : m_readoutUnits)
        {
            std::cout << "Rank: " << unit.rank << ", ID: " << unit.id << std::endl;
        }

        std::cout << "\nBUs:" << std::endl;
        for (const auto &unit : m_builderUnits)
        {
            std::cout << "Rank: " << unit.rank << ", ID: " << unit.id << std::endl;
        }
    }
}

void ContinuousBenchmark::warmupCommunication(std::vector<std::pair<int, int>> subarrayIndices, int ruRank, int buRank)
{
    std::size_t subarrayCount = subarrayIndices.size();
    std::size_t subarraySize;

    if (m_rank == ruRank)
    {
        int8_t *bufferSnd = m_unit->getBuffer();

        for (std::size_t i = 0; i < m_warmupIterations; i++)
        {
            for (std::size_t i = 0; i < subarrayCount; i++)
            {
                subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
                MPI_Send(bufferSnd + subarrayIndices[i].first, subarraySize, MPI_BYTE, buRank, 0, MPI_COMM_WORLD);
            }
        }
    }
    else if (m_rank == buRank)
    {
        int8_t *bufferRcv = m_unit->getBuffer();

        for (std::size_t i = 0; i < m_warmupIterations; i++)
        {
            for (std::size_t i = 0; i < subarrayCount; i++)
            {
                subarraySize = subarrayIndices[i].second - subarrayIndices[i].first + 1;
                MPI_Recv(bufferRcv + subarrayIndices[i].first, subarraySize, MPI_BYTE, ruRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            }
        }
    }
}

void ContinuousBenchmark::performWarmup()
{
    int buRank;
    timespec startTime, endTime;
    double throughput;
    const std::size_t bufferBytes = m_unit->getBufferBytes();
    std::size_t messageSize = bufferBytes / 10;
    std::size_t transferredSize = 0;

    std::vector<std::pair<int, int>> subarrayIndices = findSubarrayIndices(bufferBytes);

    clock_gettime(CLOCK_MONOTONIC, &startTime);

    for (int ruRank = 0; ruRank < m_nodesCount; ruRank++)
    {
        buRank = (1 + ruRank) % m_nodesCount;
        if (m_rank == ruRank || m_rank == buRank)
        {
            std::pair<std::size_t, std::size_t> result = CommunicationInterface::blockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                                       messageSize, m_warmupIterations);
            transferredSize += result.second;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &endTime);
    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, transferredSize, m_warmupIterations);

    if (m_rank == 0)
        std::cout << "\nAvg. pre-warmup throughput: " << throughput << " Mbit/s" << std::endl
                  << "Performing warmup...";

    for (int ruRank = 0; ruRank < m_nodesCount; ruRank++)
    {
        buRank = (1 + ruRank) % m_nodesCount;
        if (m_rank == ruRank || m_rank == buRank)
            warmupCommunication(subarrayIndices, ruRank, buRank);
    }

    if (m_rank == 0)
        std::cout << "Done." << std::endl;

    transferredSize = 0;
    clock_gettime(CLOCK_MONOTONIC, &startTime);

    for (int ruRank = 0; ruRank < m_nodesCount; ruRank++)
    {
        buRank = (1 + ruRank) % m_nodesCount;
        if (m_rank == ruRank || m_rank == buRank)
        {
            std::pair<std::size_t, std::size_t> result = CommunicationInterface::blockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                                       messageSize, m_warmupIterations);
            transferredSize += result.second;
        }
    }
    clock_gettime(CLOCK_MONOTONIC, &endTime);

    std::tie(std::ignore, throughput) = calculateThroughput(startTime, endTime, transferredSize, m_warmupIterations);

    if (m_rank == 0)
        std::cout << "Avg. post-warmup throughput: " << throughput << " Mbit/s\n"
                  << std::endl;
}

void ContinuousBenchmark::performPeriodicalLogging()
{
    double avgThroughput = (m_totalTransferredSize * 8.0) / (m_messagesPerPhase * m_totalElapsedTime * 1e6);

    std::cout << std::fixed << std::setprecision(2);

    std::cout << "Average throughput in " << m_lastAvgCalculationInterval << "s: " << avgThroughput << " Mbit/s" << std::endl
              << std::endl;

    std::ofstream outputFile(m_avgThroughputFilepath, std::ios::app);
    if (outputFile.is_open())
    {
        outputFile.seekp(0, std::ios::end);
        if (outputFile.tellp() == 0)
        {
            outputFile << "timestamp,comm_type,message_size,throughput\n"; // File is empty, add the header
        }

        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        outputFile << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << ","
                   << communicationTypeToString(m_commType) << ","
                   << messageSizeToString(m_commType, m_messageSize) << ","
                   << avgThroughput << "\n";

        outputFile.close();
    }
    else
    {
        std::cerr << "Failed to open file: " << m_avgThroughputFilepath << std::endl;
    }
}

void ContinuousBenchmark::performPhaseLogging(std::string ruId, std::string buId, int phase,
                                              double throughput, std::size_t errors, double averageRtt = -1)
{
    // printing
    std::cout << std::fixed << std::setprecision(8);
    std::cout << std::right << std::setw(7) << "Phase"
              << " | " << std::setw(7) << "RU"
              << " | " << std::setw(7) << "BU";

    if (m_commType == COMM_FIXED_BLOCKING || m_commType == COMM_FIXED_NONBLOCKING)
        std::cout << " | " << std::setw(14) << " Avg. RTT";

    std::cout << " | " << std::setw(25) << "Throughput"
              << " | " << std::setw(10) << " Errors"
              << std::endl;

    std::cout << std::right << std::setw(7) << phase
              << " | " << std::setw(7) << ruId
              << " | " << std::setw(7) << buId;

    if (m_commType == COMM_FIXED_BLOCKING || m_commType == COMM_FIXED_NONBLOCKING)
        std::cout << " | " << std::setw(12) << averageRtt << " s";

    std::cout << " | " << std::setw(18) << std::fixed << std::setprecision(2) << throughput << " Mbit/s"
              << " | " << std::setw(10) << errors << std::endl
              << std::endl;

    // logging
    std::ofstream outputFile(m_phasesFilepath, std::ios::app);
    if (outputFile.is_open())
    {
        outputFile.seekp(0, std::ios::end);
        if (outputFile.tellp() == 0)
        {
            outputFile << "timestamp,comm_type,message_size,phase,ru,bu,avg_rtt,throughput,errors\n"; // File is empty, add the header
        }

        std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());

        outputFile << std::put_time(std::localtime(&now), "%Y-%m-%d %H:%M:%S") << ","
                   << communicationTypeToString(m_commType) << ","
                   << messageSizeToString(m_commType, m_messageSize) << ","
                   << phase << ","
                   << ruId << ","
                   << buId << ",";
        if (m_commType == COMM_FIXED_BLOCKING || m_commType == COMM_FIXED_NONBLOCKING)
            outputFile << std::fixed << std::setprecision(8) << averageRtt;
        outputFile << "," << std::fixed << std::setprecision(1) << throughput << ","
                   << errors << "\n";

        outputFile.close();
    }
    else
    {
        std::cerr << "Failed to open file: " << m_phasesFilepath << std::endl;
    }
}

void ContinuousBenchmark::handleAverageThroughput(std::size_t transferredSize, double currentRunTimeDiff, timespec endTime)
{
    timespec lastAvgCalculationDiff = diff(m_lastAvgCalculationTime, endTime);
    double secsFromLastAvg = lastAvgCalculationDiff.tv_sec + (lastAvgCalculationDiff.tv_nsec / 1e9);
    if (secsFromLastAvg >= m_lastAvgCalculationInterval) 
    {
        if (m_rank == 0)
            performPeriodicalLogging();
        m_totalTransferredSize = 0;
        m_totalElapsedTime = 0.0;
        clock_gettime(CLOCK_MONOTONIC, &m_lastAvgCalculationTime);
    }
    else
    {
        std::size_t tmpTransferredSize = 0;
        double tmpElapsedTime = 0.0;

        for (const auto &unit : m_builderUnits)
        {
            int buRank = unit.rank;

            if (m_rank == buRank && m_rank != 0)
            {
                MPI_Send(&transferredSize, 1, MPI_UNSIGNED_LONG_LONG, 0, 0, MPI_COMM_WORLD);
                MPI_Send(&currentRunTimeDiff, 1, MPI_DOUBLE, 0, 0, MPI_COMM_WORLD);
            }
            else if (m_rank == 0)
            {
                MPI_Recv(&tmpTransferredSize, 1, MPI_UNSIGNED_LONG_LONG, buRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
                MPI_Recv(&tmpElapsedTime, 1, MPI_DOUBLE, buRank, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

                m_totalTransferredSize += tmpTransferredSize;
                m_totalElapsedTime += tmpElapsedTime;
            }
        }
        if (m_rank == 0) // case when 0 is one of the BUs
        {
            m_totalTransferredSize += transferredSize;
            m_totalElapsedTime += currentRunTimeDiff;
        }
    }
}

void ContinuousBenchmark::run()
{
    int ruRank, buRank;
    std::string ruId, buId;

    timespec startTime, endTime;

    std::pair<std::size_t, std::size_t> result = std::make_pair(0, 0);

    for (int phase = 0; phase < m_nodesCount / 2; phase++)
    {
        MPI_Barrier(MPI_COMM_WORLD);
        if (m_rank == 0)
            std::cout << "\n\n============================================================\n\n"
                      << std::endl;

        if (m_unit->getUnitType() == UnitType::RU)
        {
            ruRank = m_rank;
            ruId = m_unit->getId();
            buRank = m_builderUnits.at(m_unit->getPair(phase)).rank;
            buId = (buRank == -1) ? "DUMMY" : m_builderUnits.at(m_unit->getPair(phase)).id;
        }
        else if (m_unit->getUnitType() == UnitType::BU)
        {
            buRank = m_rank;
            buId = m_unit->getId();
            ruRank = m_readoutUnits.at(m_unit->getPair(phase)).rank;
            ruId = (ruRank == -1) ? "DUMMY" : m_readoutUnits.at(m_unit->getPair(phase)).id;
        }

        // perform communication
        timespec elapsedTime;
        std::size_t errorMessageCount = 0;
        std::size_t transferredSize = 0;
        double currentRunTimeDiff = 0.0;
        clock_gettime(CLOCK_MONOTONIC, &startTime);

        if (ruRank != -1 && buRank != -1) // skip communication involving dummy nodes
        {
            for (int message = 0; message < m_messagesPerPhase; message++)
            {
                if (m_commType == COMM_FIXED_BLOCKING)
                {
                    result = CommunicationInterface::blockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                           m_messageSize, m_iterations);
                }
                else if (m_commType == COMM_FIXED_NONBLOCKING)
                {
                    result = CommunicationInterface::nonBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                              m_messageSize, m_iterations, m_syncIterations);
                }
                else if (m_commType == COMM_VARIABLE_BLOCKING)
                {
                    result = CommunicationInterface::variableBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                   m_messageSizes, m_iterations);
                }
                else if (m_commType == COMM_VARIABLE_NONBLOCKING)
                {
                    result = CommunicationInterface::variableNonBlockingCommunication(m_unit.get(), ruRank, buRank, m_rank,
                                                                                      m_messageSizes, m_iterations, m_syncIterations);
                }

                // perform logging and reset result variable
                if (m_rank == buRank)
                {
                    errorMessageCount += result.first;
                    transferredSize += result.second;

                    result = std::make_pair(0, 0);
                }
                else if (m_rank == ruRank)
                {
                    result = std::make_pair(0, 0);
                }
            }
        }

        clock_gettime(CLOCK_MONOTONIC, &endTime);

        elapsedTime = diff(startTime, endTime);
        currentRunTimeDiff += (elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9));

        if ((m_rank == buRank) || (buRank == -1))
        {
            if (ruRank == -1 || buRank == -1)
            {
                performPhaseLogging(ruId, buId, phase, 0, 0);
            }
            else if (m_commType == COMM_VARIABLE_BLOCKING || m_commType == COMM_VARIABLE_NONBLOCKING)
            {
                double avgThroughput = (transferredSize * 8.0) / (currentRunTimeDiff * 1e6);
                performPhaseLogging(ruId, buId, phase, avgThroughput, errorMessageCount);
            }
            else if (m_commType == COMM_FIXED_BLOCKING || m_commType == COMM_FIXED_NONBLOCKING)
            {
                double avgThroughput = (transferredSize * 8.0) / (currentRunTimeDiff * 1e6);
                double averageRtt = currentRunTimeDiff / (m_iterations * m_messagesPerPhase);
                performPhaseLogging(ruId, buId, phase, avgThroughput, errorMessageCount, averageRtt);
            }
        }

        handleAverageThroughput(transferredSize, currentRunTimeDiff, endTime);
    }
}
