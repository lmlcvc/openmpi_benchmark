#include "variable_message.h"

BenchmarkVariableMessage::BenchmarkVariableMessage(std::vector<ArgumentEntry> args, int rank, int size, CommunicationType commType)
{
    m_rank = rank;
    m_nodesCount = size;
    m_commType = commType;

    m_unit = std::make_unique<Unit>();

    BenchmarkVariableMessage::parseArguments(args);

    m_syncIterations = m_iterations / 1e4;

    initUnitLists();
    m_unit->allocateMemory();

    initMessageSizes();

    if (m_rank == 0)
    {
        std::cout << std::endl
                  << "Performing variable size benchmark. "
                  << std::endl;

        if (commType == COMM_VARIABLE_BLOCKING)
        {
            std::cout << "Blocking communication." << std::endl
                      << std::endl;
        }
        else if (commType == COMM_VARIABLE_NONBLOCKING)
        {
            std::cout << "Non-blocking communication." << std::endl
                      << std::endl;
        }
        std::cout << "Available sizes: " << m_messageSizeVariants << " (range: 10000 B - " << m_ruBufferBytes << " B)" << std::endl;

        std::cout << std::endl
                  << std::left << std::setw(20) << "RU buffer size:"
                  << std::right << std::setw(10) << m_ruBufferBytes << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "BU buffer size:"
                  << std::right << std::setw(10) << m_buBufferBytes << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "Number of iterations:"
                  << std::right << std::setw(9) << m_iterations << std::endl;
    }

    clock_gettime(CLOCK_MONOTONIC, &m_lastAvgCalculationTime);
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
                m_ruBufferBytes = tmp;
            break;
        case 'b':
            tmp = std::stoul(entry.value);
            if (tmp > 0)
                m_buBufferBytes = tmp;
            break;
        case 'm':
            tmp = std::stoul(entry.value);
            m_messageSizeVariants = (tmp > 0) ? tmp : m_messageSizeVariants;
            break;
        case 'p':
            tmp = std::stoul(entry.value);
            m_messagesPerPhase = (tmp > 0) ? tmp : m_messagesPerPhase;
            break;
        case 'i':
            tmp = std::stoul(entry.value);
            m_iterations = (tmp >= m_minIterations) ? tmp : m_iterations;
            break;
        case 'w':
            tmp = std::stoul(entry.value);
            m_warmupIterations = (tmp > 0) ? tmp : m_warmupIterations;
            break;
        case 'l':
            tmp = std::stoul(entry.value);
            m_lastAvgCalculationInterval = (tmp > 0) ? tmp : m_lastAvgCalculationInterval;
            break;
        case 'c':
            m_unit->setConfigPath(entry.value);
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
    std::size_t upperBound = m_ruBufferBytes;

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
