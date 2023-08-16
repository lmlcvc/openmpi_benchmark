#include "variable_message.h"

BenchmarkVariableMessage::BenchmarkVariableMessage(std::vector<ArgumentEntry> args, int rank)
{
    m_rank = rank;
    parseArguments(args);

    initMessageSizes();

    /*m_sndBufferBytes = m_sndBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower)); // XXX: check buffer size implementation
    m_rcvBufferBytes = m_rcvBufferSize * static_cast<std::size_t>(std::pow(2, m_maxPower));

    allocateMemory();*/

    // TODO: setup buffers
}

void BenchmarkVariableMessage::initMessageSizes()
{
    unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
    std::mt19937 generator(seed);

    int lowerBound = 1e4;
    int upperBound = 1e7;

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
