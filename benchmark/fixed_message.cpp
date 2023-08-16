#include "fixed_message.h"

BenchmarkFixedMessage::BenchmarkFixedMessage(std::vector<ArgumentEntry> args, int rank, CommunicationType commType)
{
    m_rank = rank;
    m_commType = commType;
    m_messageSize = 1e5;

    parseArguments(args);

    allocateMemory();

    if (m_rank == 0 && m_messageSize > m_sndBufferBytes)
    {
        std::cerr << "Message cannot exceed buffer. Exiting." << std::endl;
        MPI_Finalize();
        std::exit(1);
    }

    if (m_rank == 0)
    {
        std::cout << std::endl
                  << "Performing fixed size benchmark." << std::endl << std::endl;

        std::cout << std::left << std::setw(20) << "Message size:"
                  << std::right << std::setw(10) << m_messageSize << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "Send buffer size:"
                  << std::right << std::setw(10) << m_sndBufferBytes << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "Receive buffer size:"
                  << std::right << std::setw(10) << m_rcvBufferBytes << " B" << std::endl;

        std::cout << std::left << std::setw(20) << "Number of iterations:"
                  << std::right << std::setw(9) << m_iterations << std::endl;
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

void BenchmarkFixedMessage::printIterationInfo(timespec startTime, timespec endTime, std::size_t transferredSize)
{
}

void BenchmarkFixedMessage::run()
{
}
