#include "variable_message.h"

BenchmarkVariableMessage::BenchmarkVariableMessage(std::vector<ArgumentEntry> args, int rank)
{
    m_rank = rank;
    parseArguments(args);

    initMessageSizes();
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

    int expectedValue = 1 << 18;
    int shiftFactor = expectedValue - (upperBound + lowerBound) / 2;

    std::uniform_int_distribution<int> distribution(lowerBound + shiftFactor, upperBound + shiftFactor);

    m_messageSizes.clear();
    for (size_t i = 0; i < 100; i++)
        m_messageSizes.push_back(distribution(generator));
}

void BenchmarkVariableMessage::run()
{
}
