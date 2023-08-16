#include "fixed_message.h"

BenchmarkFixedMessage::BenchmarkFixedMessage(std::vector<ArgumentEntry> args, int rank)
{
    m_rank = rank;
    parseArguments(args);

    allocateMemory();

    if (m_messageSize > m_sndBufferBytes)
    {
        std::cerr << "Message cannot exceed buffer. Exiting." << std::endl;
        MPI_Finalize();
        std::exit(1);
    }
}

void BenchmarkFixedMessage::parseArguments(std::vector<ArgumentEntry> args)
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
            m_messageSize = (tmp > 0) ? tmp : m_messageSize;
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

void BenchmarkFixedMessage::run()
{
}
