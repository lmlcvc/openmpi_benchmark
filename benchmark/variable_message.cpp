#include "variable_message.h"

BenchmarkVariableMessage::BenchmarkVariableMessage(std::vector<ArgumentEntry> args, int rank, CommunicationType commType)
{
    m_rank = rank;
    m_commType = commType;

    parseArguments(args);

    initMessageSizes();

    if (m_rank == 0)
    {
        std::cout << std::endl
                  << "Performing variable size benchmark." << std::endl
                  << "Available sizes: " << m_messageSizeVariants << " (range: 10000 B - " << m_sndBufferBytes << " B" << std::endl;

        std::cout << std::endl
                  << std::left << std::setw(20) << "Send buffer size:"
                  << std::right << std::setw(10) << m_sndBufferBytes << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "Receive buffer size:"
                  << std::right << std::setw(10) << m_rcvBufferBytes << " B" << std::endl;

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
        case 'b':
            tmp = std::stoul(entry.value);
            m_sndBufferBytes = (tmp > 0) ? tmp : m_sndBufferBytes;
            break;
        case 'r':
            tmp = std::stoul(entry.value);
            m_rcvBufferBytes = (tmp > 0) ? tmp : m_rcvBufferBytes;
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

    int lowerBound = 1e4;
    int upperBound = m_sndBufferBytes;

    int expectedValue = 1 << 18; // FIXME: not if sndBufferBytes is something else, do it with shifts
    // also print that
    int shiftFactor = expectedValue - (upperBound + lowerBound) / 2;

    std::uniform_int_distribution<int> distribution(lowerBound + shiftFactor, upperBound + shiftFactor);

    m_messageSizes.clear();
    for (size_t i = 0; i < 100; i++)
        m_messageSizes.push_back(distribution(generator));
}

void BenchmarkVariableMessage::printIterationInfo(timespec startTime, timespec endTime, std::size_t transferredSize, std::size_t errorMessagesCount)
{
    if (m_rank)
        return;

    timespec elapsedTime = diff(startTime, endTime);
    double elapsedSecs = elapsedTime.tv_sec + (elapsedTime.tv_nsec / 1e9);

    double avgThroughput = (transferredSize * 8.0) / (elapsedSecs * 1e6);

    std::cout << std::fixed << std::setprecision(8);

    std::cout << std::right << std::setw(25) << "Throughput"
              << " | " << std::setw(10) << " Errors"
              << std::endl;

    std::cout << std::right << std::setw(18) << std::fixed << std::setprecision(2) << avgThroughput << " Mbit/s"
              << " | " << std::setw(10) << errorMessagesCount << std::endl
              << std::endl;
}

void BenchmarkVariableMessage::run()
{
}
