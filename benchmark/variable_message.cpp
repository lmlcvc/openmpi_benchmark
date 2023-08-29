#include "variable_message.h"

BenchmarkVariableMessage::BenchmarkVariableMessage(std::vector<ArgumentEntry> args, int rank, int size, CommunicationType commType)
{
    m_rank = rank;
    m_nodesCount = size;
    m_commType = commType;

    m_readoutUnit = std::make_unique<ReadoutUnit>(rank);
    m_builderUnit = std::make_unique<BuilderUnit>(rank);

    parseArguments(args);

    m_syncIterations = m_iterations / 1e4;

    m_readoutUnit->allocateMemory();
    m_builderUnit->allocateMemory();

    initMessageSizes();

    if (m_rank == 0)
    {
        std::cout << std::endl
                  << "Performing variable size benchmark." << std::endl
                  << "Available sizes: " << m_messageSizeVariants << " (range: 10000 B - " << m_readoutUnit->getBufferBytes() << " B)" << std::endl;

        std::cout << std::endl
                  << std::left << std::setw(20) << "RU buffer size:"
                  << std::right << std::setw(10) << m_readoutUnit->getBufferBytes() << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "BU buffer size:"
                  << std::right << std::setw(10) << m_builderUnit->getBufferBytes() << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "Number of iterations:"
                  << std::right << std::setw(9) << m_iterations << std::endl;
    }
}

void BenchmarkVariableMessage::parseArguments(std::vector<ArgumentEntry> args)
{
    std::size_t tmp;
    for (const auto &entry : args)
    {
        switch (entry.option)
        {
        case 'r':
            tmp = std::stoul(entry.value);
            if (tmp > 0)
                m_readoutUnit->setBufferBytes(tmp);
            break;
        case 'b':
            tmp = std::stoul(entry.value);
            if (tmp > 0)
                m_builderUnit->setBufferBytes(tmp);
            break;
        case 'm':
            tmp = std::stoul(entry.value);
            m_messageSizeVariants = (tmp > 0) ? tmp : m_messageSizeVariants;
            break;
        case 'i':
            tmp = std::stoul(entry.value);
            m_iterations = (tmp > m_minIterations) ? tmp : m_iterations;
            break;
        case 'w':
            tmp = std::stoul(entry.value);
            m_warmupIterations = (tmp > 0) ? tmp : m_warmupIterations;
            break;
        default:
            if (m_rank == 0)
            {
                std::cerr << "Invalid argument: " << entry.option << std::endl;
                MPI_Finalize();
                std::exit(1);
            }
        }
    }
}

void BenchmarkVariableMessage::initMessageSizes()
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 generator(seed);

    std::size_t lowerBound = 1e4;
    std::size_t upperBound = m_readoutUnit->getBufferBytes();

    if (upperBound < lowerBound)
    {
        std::cout << "Send buffer size must be larger than 10000 B. Exiting.";
        MPI_Finalize();
        std::exit(1);
    }

    std::uniform_real_distribution<double> distribution(0.0, 1.0);

    m_messageSizes.clear();
    for (size_t i = 0; i < m_messageSizeVariants; i++)
    {
        double randomValue = distribution(generator);
        std::size_t messageSize = std::round(randomValue * (upperBound - lowerBound) + lowerBound);
        m_messageSizes.push_back(messageSize);
    }
}

void BenchmarkVariableMessage::printIterationInfo(timespec startTime, timespec endTime, int ruRank, int buRank,
                                                  std::size_t transferredSize, std::size_t errorMessagesCount)
{
    timespec elapsedTime = diff(startTime, endTime);
    double elapsedSecs = elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9);

    double avgThroughput = (transferredSize * 8.0) / (elapsedSecs * 1e6);

    std::cout << std::fixed << std::setprecision(8);

    std::cout << std::right << std::setw(7) << "Phase"
              << " | " << std::setw(7) << "RU"
              << " | " << std::setw(7) << "BU"
              << " | " << std::setw(25) << "Throughput"
              << " | " << std::setw(10) << " Errors"
              << std::endl;

    std::cout << std::right << std::setw(7) << m_currentPhase
              << " | " << std::setw(7) << ruRank
              << " | " << std::setw(7) << buRank
              << " | " << std::setw(18) << std::fixed << std::setprecision(2) << avgThroughput << " Mbit/s"
              << " | " << std::setw(10) << errorMessagesCount << std::endl
              << std::endl;
}
